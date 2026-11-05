#include <cstdio>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <ctime>
#include <filesystem>
#include <cstring>
#include <string>
#include <vector>
#include <iomanip>

using namespace std;

#define LIMIT 10000 // Number of limiting lines for reading a file
#define N_UPDATES 1000 // it means that it trims the db after 1000 calls
#define BYTES_CHECK (200*(LIMIT + N_UPDATES))

//trim the history file
void trim_history(const string& file){
    vector<string> lines;
    ifstream in(file);
    string s;

    while(getline(in, s))
      lines.emplace_back(s);

    in.close();
    if( lines.size() <= LIMIT + N_UPDATES ) 
      return;

    string tmp = file + ".tmp";
    ofstream out(tmp);

    for(size_t i=lines.size() - LIMIT; i<lines.size(); i++)
      out << lines[i] << "\n";

    out.close();
    filesystem::rename(tmp, file); // atomic: never a half-written history
}

// These functions are needed for the bash part
const char* BASH_INIT = R"BASH(
if [[ $- == *i* ]]; then
    _nvim2_alt(){
        [[ $READLINE_LINE == nvim2\ * ]] && READLINE_LINE+=" --find"
        READLINE_POINT=${#READLINE_LINE}
    }
    bind -x '"\C-x\C-f": _nvim2_alt'
    bind '"\e\C-m": "\C-x\C-f\C-m"'
fi
)BASH";

const char* ZSH_INIT = R"ZSH(
_nvim2_alt(){
    [[ $BUFFER == nvim2\ * ]] && BUFFER+=" --find"
    zle accept-line
}
zle -N _nvim2_alt
bindkey '^[^M' _nvim2_alt
)ZSH";

int main(int argc, char **argv){
  //NOTE: what if one wants vim? so the stats are for vim and nvim, mhh no fells like if une changes the command is better
    // to have access to the same dataset. maybe a flag that signals that the user used vim rhather than nvim or viceversa
  //NOTE: multiple files should count as multiple files not one
  //BUG: Creating a file whose name matches something. nv main when you actually want to create a new file main will open ./main.cpp instead. That is why you need anther method.

  //NOTE: This checks the bash part from bashrc and executes the scripts abpve
  if(argc == 3 && strcmp(argv[1], "--init") == 0){
     if(strcmp(argv[2], "bash") == 0)
      cout << BASH_INIT;
     else if(strcmp(argv[2], "zsh") == 0)
      cout << ZSH_INIT;
     else{
         cerr << "unsupported shell: " << argv[2] << "\n";
          return 1;
        }
     return 0;
  }
  // finding home
  const char* env = getenv("HOME");
  if(!env){
        cerr << "HOME not set. You need to properly set home before using this program." << endl;
        exit(1);
  }
  filesystem::path home(env); // the var name is home

    //get current time of usage   
  time_t timestamp;
  time(&timestamp);
  string default_cmd = "nvim";
  // Neovim flags whose next argument is a value, not a file
  const char* VALUE_FLAGS[] = {"-c", "--cmd", "-u", "-i", "-t", "-q", "-s", "-w", "-W", "-S", "--listen", "--startuptime"};

  string hist_file = home.string() + "/.local/share/nvim2/history_nvim.csv";
  if(filesystem::exists(hist_file) && filesystem::file_size(hist_file) > BYTES_CHECK)
    trim_history(hist_file);

  //write to storage
  ofstream history_file( hist_file, ios_base::app );
  if(!history_file.is_open()){
    cerr << "ERROR: Could not open history file for writing." << endl;
    exit(1);
  }

  // dashboard call no argumet
  if ( argc < 2 ){
    history_file << put_time(localtime(&timestamp), "%a %b %d %H:%M:%S %Y") << "\t" << filesystem::current_path().string() << "\t" << "NO_ARG" << "\n";
    history_file.close();
    char *args[] = {default_cmd.data(), nullptr};
    execvp(default_cmd.c_str(), args);
    perror( ("execvp "+default_cmd).c_str() );
    exit(127);
  }

  // saves the argument and checks for the '-' or '--' in the 
  string appo_string;
  vector<char*> total_arg;
  total_arg.reserve( argc + 1 );
  total_arg.emplace_back( nullptr ); //we will overwrite with argv[0]
  vector<string> file_name;

  bool only_files = false;//LOL

  for (int i = 1; i < argc; i++){
    if(argv[i][0] == '\0') // empty argument then argv[i][0] is the closing '\0' so is safe
      continue;

    if ( strcmp(argv[i], "--vim") == 0 ){
      default_cmd = "vim";
      continue;
    }else if( strcmp(argv[i], "--init") == 0 ){

    //TODO: goddamn it will do smth
    
    }

    total_arg.emplace_back( argv[i] );
    if( only_files ){
      file_name.emplace_back( argv[i] );
      continue;
    }

    if( strcmp(argv[i], "--") == 0 ){
      only_files = true;
      continue;
    }
    
    int count = 0; //here we count the number of flags that accept a value instead of a file   
    for(const char*& f : VALUE_FLAGS)
        if( strcmp(argv[i], f) == 0 ) count++;

    if( count != 0 && i+1 < argc ){ // this gets the correct file names
      total_arg.emplace_back( argv[++i] );
      continue;
    }

    if( argv[i][0] == '-' || argv[i][0] == '+' )
      continue; // flag or +cmd
    
    file_name.emplace_back( argv[i] );
  }

  total_arg[0] = default_cmd.data();
 
  for (size_t i = 1; i < total_arg.size() - 1; i++ ){
     appo_string += total_arg[i];
     appo_string += " ";
  }
  appo_string += total_arg[total_arg.size() - 1 ];

  //string current_path = filesystem::current_path().string();
  // now we can check if file_name is empty or not, if it is empty we will write the current path, otherwise we will write the files
  if( file_name.empty() )
    history_file << put_time(localtime(&timestamp), "%a %b %d %H:%M:%S %Y") << "\t" << filesystem::current_path().string() << "\t" << "NO_ARG\n";
  else{
    for( const string& f : file_name)
      history_file << put_time(localtime(&timestamp), "%a %b %d %H:%M:%S %Y") << "\t" << filesystem::weakly_canonical( filesystem::current_path() / f ).string() << "\t" << appo_string << "\n";
  }

  history_file.close();
  total_arg.emplace_back( nullptr ); //expected by excvp
  
  execvp( default_cmd.c_str(), total_arg.data() );
  perror( ("execvp "+default_cmd).c_str() );
  exit(127);

  return 0;

};

