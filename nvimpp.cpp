#include <cctype>
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
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cmath>

using namespace std;

unsigned int LIMIT = 10000; // Number of limiting lines for reading a file
unsigned int N_UPDATES = 1000; // it means that it trims the db after 1000 calls
unsigned int MENU_ROWS = 15; // max rows of the inline picker list
unsigned int HL = 7*86400; // half-life in seconds
unsigned int BYTES_CHECK = (200*(LIMIT + N_UPDATES)); // Rough estimate of the bytesize of the file

double weight(long deltaTime, double halflife){
    return pow(0.5,(deltaTime)/halflife);
}

void read_config(string home){

  string config_file = home + "/.config/nvimpp/nvimpp.config";
  ifstream in(config_file);
  if(!in.is_open()){
    cerr << "\033[1;38;5;208mWARNING!!\033[0m Cannot read config file. Check the path to the config. Reverting to default values.\n";
  }

  string line;
  string key;
  unsigned int value;
  string equal;

  unordered_map<string, unsigned int> config;

  while(getline(in, line)){
    if(line.empty() || line[0] == '#') //avoid empty lines and make comments available
      continue;
    
    istringstream ss( line );
    ss >> key >> equal >> value;
    config[key] = value;

   }

  //These values HAVE to be different from 0 otherwise will wipe out the history, if false defaluts to the defalut values above
  if(config.count("limit")) LIMIT = config["limit"];
  if(config.count("wait")) N_UPDATES = config["wait"];
  if(config.count("rows")) MENU_ROWS = config["rows"];
  if(config.count("halflife_days")) HL = config["halflife_days"]*86400;
  BYTES_CHECK = (200*(LIMIT + N_UPDATES)); // Rough estimate of the bytesize of the file

}


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
    _nvimpp_alt(){
        [[ $READLINE_LINE == nvimpp\ * ]] || [[ $READLINE_LINE == nvim\ * ]] && READLINE_LINE+=" --find"
        READLINE_POINT=${#READLINE_LINE}
    }
    bind -x '"\C-x\C-f": _nvimpp_alt'
    bind '"\e\C-m": "\C-x\C-f\C-m"'
fi
)BASH";

const char* ZSH_INIT = R"ZSH(
_nvimpp_alt(){
    [[ $BUFFER == nvimpp\ * ]] || [[ $BUFFER == nvim\ * ]] && BUFFER+=" --find"
    zle accept-line
}
zle -N _nvimpp_alt
bindkey '^[^M' _nvimpp_alt
)ZSH";

// Stuff for --find
string lower(string s){

  transform(s.begin(), s.end(), s.begin(), ::tolower);
  return s;
}

//find the best match
bool find_matches(const string& path, const vector<string>& query ){
  size_t pos = 0;
  string appo = lower(path);

  for (const string& elem : query ){
    pos = appo.find( lower(elem), pos ) ; // FIND RETURNS AN ITERATOR
    if ( pos == string::npos )
      return false;
    pos += elem.size(); // THE STRING SIZE
  }
  
  // LAst match is the file name 
  string name = lower( filesystem::path(path).filename().string() );

  return name.find( lower(query.back()) ) != string::npos;
}

// highest frecency path in the history that matches the query and still exists, basically nvimi
string find_best(const string& file, const vector<string>& query){

  ifstream in(file);
  string line, best;
  unordered_map<string, double> score;
  auto now = chrono::system_clock::now();
  
  while( getline(in, line) ){
    istringstream ss(line);
    string path, raw;
    tm tm = {};
    ss >> get_time(&tm, "%a %b %d %H:%M:%S %Y");
    if(ss.fail())
      continue;
    getline(ss >> ws, path, '\t');
    getline(ss, raw);
    if(raw == "NO_ARG")
      continue; // folders, not files
    tm.tm_isdst = -1;
    auto deltatime = chrono::duration_cast<chrono::seconds>( now - chrono::system_clock::from_time_t( mktime(&tm) )).count();
    score[path] += weight( deltatime, HL );
  }

  double top = -1;

  for(auto& [path, s] : score){
    if(s > top && find_matches(path, query) && filesystem::exists(path)){
      top = s;
      best = path; 
    }
  }

  return best;
}
///////////////////////////////
// MAIN
///////////////////////////////


int main(int argc, char **argv){
  //NOTE: what if one wants vim? so the stats are for vim and nvim, mhh no fells like if une changes the command is better
    // to have access to the same dataset. maybe a flag that signals that the user used vim rhather than nvim or viceversa
  //NOTE: multiple files should count as multiple files not one
  //NOTE: This checks the bash part from bashrc and executes the scripts abpve
  //
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

read_config(home.string());
    //get current time of usage   
  time_t timestamp;
  time(&timestamp);
  string default_cmd = "nvim";

  // Neovim flags whose next argument is a value, not a file
  const char* VALUE_FLAGS[] = {"-c", "--cmd", "-u", "-i", "-t", "-q", "-s", "-w", "-W", "-S", "--listen", "--startuptime"};

  string hist_file = home.string() + "/.local/share/nvimpp/history_nvim.csv";
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
  string best;
  vector<char*> total_arg;
  total_arg.reserve( argc + 1 );
  total_arg.emplace_back( nullptr ); //we will overwrite with argv[0]
  vector<string> file_name;

  bool only_files = false;//LOL
  bool search_mode = false;

  for (int i = 1; i < argc; i++){
    if(argv[i][0] == '\0') // empty argument then argv[i][0] is the closing '\0' so is safe
      continue;

    if ( strcmp(argv[i], "--vim") == 0 ){
      default_cmd = "vim";
      continue;
    }else if( strcmp(argv[i], "--init") == 0 ){}

    if ( strcmp(argv[i], "--find") == 0 ){
      search_mode = true;
      continue;
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

  if(search_mode){
    if(file_name.empty()){
        cerr << "nvimpp: --find needs a query\n";
        exit(1);
      }
    best = find_best(hist_file, file_name);
    if(best.empty()){
        cerr << "nvimpp: no match for the query\n";
        exit(1);
      }
    // drop the query words from the Neovim arguments and keep the flags
    total_arg.erase(remove_if(total_arg.begin() + 1, total_arg.end(), [&](char* a){
        return find(file_name.begin(), file_name.end(), string(a)) != file_name.end();
      }), total_arg.end());

    total_arg.emplace_back(best.data());
    file_name = {best};
  }

  //set the name of the program to open
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
