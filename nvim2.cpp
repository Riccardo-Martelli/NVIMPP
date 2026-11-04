#include <cstdio>
#include <iostream>
#include <fstream>
#include <ios>
#include <cstdlib>
#include <unistd.h>
#include <ctime>
#include <filesystem>
#include <getopt.h>
#include <cstring>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

using namespace std;

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
  cout << home.string();


    //get current time of usage   
  time_t timestamp;
  time(&timestamp);
  string default_cmd = "nvim";

  //write to storage
  ofstream history_file( home.string() + "/.local/share/nvim2/history_nvim.csv", ios_base::app );

  // dashboard call no argumet
  if ( argc < 2 ){
    history_file << put_time(localtime(&timestamp), "%a %b %d %H:%M:%S %Y") << " " << filesystem::current_path().string() << " " << "NO_ARG" << "\n";
    history_file.close();
    char *args[] = {default_cmd.data(), nullptr};
    execvp(default_cmd.c_str(), args);
    perror( ("execvp"+default_cmd).c_str() );
    exit(1);
  }

  // saves the argument and checks for the '-' or '--' in the 
  string appo_string;
  vector<char*> total_arg;
  total_arg.reserve( argc + 1 );
  total_arg.emplace_back( nullptr ); //we will overwrite with argv[0]
  string file_name;

  for (int i = 1; i < argc; i++){

    if ( strcmp(argv[i], "--vim") == 0 ){
      default_cmd = "vim";
      continue;
    }else if( strcmp(argv[i], "--init") == 0 ){

    //TODO: goddamn it will do smth
    }
    if ( i%2 != 0 && string_view(argv[i]).compare(0, 2, "-") != 0 )
     file_name = argv[i]; 

    total_arg.emplace_back( argv[i] );
  }

  total_arg[0] = default_cmd.data();
  string file;
 
  for (size_t i = 1; i < total_arg.size() - 1; i++ ){
     appo_string += total_arg[i];
     appo_string += " ";
  }
  appo_string += total_arg[total_arg.size() - 1 ];

  //string current_path = filesystem::current_path().string();
  history_file << put_time(localtime(&timestamp), "%a %b %d %H:%M:%S %Y") << " " << filesystem::weakly_canonical( filesystem::current_path() / file_name ).string() << " " << appo_string << "\n";
  
  history_file.close();
  total_arg.emplace_back( nullptr ); //expected by excvp
  
  execvp( default_cmd.c_str(), total_arg.data() );
  perror( ("execlp "+default_cmd).c_str() );

  return 0;

};

