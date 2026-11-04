#include <ctime>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <filesystem>

using namespace std;

#define HL 604800 // seconds in 7 days
#define LIMIT 5000 // Number of limiting lines for reading a file
#define MENU_ROWS 10 // max rows of the inline picker list

//NOTE: the default implementation is the scoring. 
//BUG: the storage has to be limited so far there is not limit 

  struct Data{
    int duration_s;
    int freq;
    float score;
    string cmd;
    string flag;
  };

double weight(long deltaTime, double halflife){
    // this takes a long integer
  // BUG: this is overcomplicarted fot not FUCKING REASON
    return exp(-log(2.0)*(deltaTime)/halflife);
}

string timeAgo(long dt){

    if(dt < 0) dt = 0; // future timestamp or clock skew

    // seconds per unit: s, min, h, d, w, mo (30 d), y (365 d)
    const long size[7] = {1, 60, 3600, 86400, 604800, 2592000, 31536000};
    const char* unit[7] = {"s", "min", "h", "d", "w", "mo", "y"};
    int i = 6;

    while(i > 0 && dt < size[i]) i--;

    return to_string(dt / size[i]) + unit[i] + " ago";
}

int pickMenu(const vector<Data>& items){

    if(items.empty()) return -1;

    int tty = open("/dev/tty", O_RDWR); // talk to the terminal directly
    termios oldt, raw;
    tcgetattr(tty, &oldt);

    raw = oldt;
    raw.c_lflag &= ~(ICANON | ECHO | ISIG); // keys one at a time, not echoed, Ctrl-C arrives as byte 3 instead of killing us
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(tty, TCSANOW, &raw);

    winsize ws;
    ioctl(tty, TIOCGWINSZ, &ws);
    int n = items.size();
    int h = min({n, MENU_ROWS, max(1, ws.ws_row - 4)}); // list rows, never more than the screen allows
    int total = h + 3; // + 3 header lines
    int avail = max(4, ws.ws_col - 17); // columns left for the command after "> ", freq, score and gaps

    // reserve the block below the prompt: print the lines (the terminal scrolls if needed), go back up, save the position
    string on = "\033[?25l" + string(total, '\n') + "\033[" + to_string(total) + "A\0337";
    write(tty, on.c_str(), on.size());
    int sel = 0, top = 0;
    char c;

    while(true){
        if(sel < top)
          top = sel;
        if(sel >= top + h)
          top = sel - h + 1; // scroll when the cursor leaves the window

        ostringstream frame;
        frame << "\0338\033[J"; // back to the start of the block and clear everything below it
        frame << "\033[1;32m" << "         NVIMI commands statistics" << "\033[0m\r\n";
        frame << "  \033[1;32m" << setw(4) << "freq" << "  \033[1;31m" << setw(7) << "score" << "  \033[1;37m" << "command" << "\033[0m\r\n"; // legend: same colors and order as the rows

        for(int i=top; i<min(n, top + h); i++){
            string bg = (i == sel ? ";47" : ""); // white background on the selected row
            string name = items[i].cmd + (items[i].flag == "NO_ARG" ? "  dashboard" : "");
            if((int)name.size() > avail) name = "..." + name.substr(name.size() - avail + 3); // keep the tail: the file name is the informative part
            frame << (i == sel ? "\033[0;30;47m> " : "\033[0m  ") << "\033[0;32" << bg << "m" << right << setw(4) << items[i].freq << "  \033[0;31" << bg << "m" << setw(7) << fixed << setprecision(2) << items[i].score << "  \033[0" << (i == sel ? ";30;47" : "") << "m" << name << "\033[0m\r\n";
        }

        string f = frame.str();
        write(tty, f.c_str(), f.size());

        if(read(tty, &c, 1) != 1){ sel = -1; break; }
        if(c == '\n' || c == '\r') break;
        if(c == 'q' || c == 3){ sel = -1; break; } // 3 = Ctrl-C
        if(c == 'k' && sel > 0) sel--;
        if(c == 'j' && sel < n-1) sel++;
        if(c == '\033'){

            char seq[2];
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 1; // wait at most 0.1 s for the rest of an arrow sequence

            tcsetattr(tty, TCSANOW, &raw);

            int k = read(tty, seq, 2);
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;

            tcsetattr(tty, TCSANOW, &raw);

            if(k <= 0){ // lone Esc
              sel = -1;
              break;
            }

            if(k == 2 && seq[0] == '['){
              if(seq[1] == 'A' && sel > 0) sel--;
              if(seq[1] == 'B' && sel < n-1) sel++;
            }
        }
    }

    string off = "\0338\033[J\033[?25h"; // erase the menu block and show the cursor again
    write(tty, off.c_str(), off.size());
    tcsetattr(tty, TCSANOW, &oldt); // restore normal terminal behavior
    close(tty);

    return sel;
}

void read_data(string file, vector<string> &file_names, chrono::duration<float> &deltaTime){
    string output;
    auto now = chrono::system_clock::now();
    ifstream in(file);

    while ( getline(in, output) ){
      istringstream ss(output);
      string output1; // name of the file
      string filepath; // name of the file
      tm tm = {}; // struct for the time
      ss >> get_time(&tm,"%a %b %d %H:%M:%S %Y") >> filepath;
      getline(ss >> ws, output1);
      tm.tm_isdst = -1; // takes care of Daylight Saving Time
      file_names.emplace_back(output1);
      deltaTime = now - chrono::system_clock::from_time_t(mktime(&tm));
      long appo = chrono::duration_cast<chrono::seconds>(deltaTime).count(); 
    }  
    
    in.close();
}

int main(int argc, char **argv){
 
  //define current time and date
  auto now = chrono::system_clock::now();
  bool stats = false;
  bool history = false;
  bool defult = true;
  
  string output;
  vector<string> file_names; 
  chrono::duration<float> deltaTime; 

  // finding home
  const char* env = getenv("HOME");
  if(!env){
        cerr << "HOME not set. You need to properly set home before using this program." << endl;
        exit(1);
  }
  filesystem::path home(env); // the var name is home
  string db_file = home / ".local/share/nvim2/history_nvim.csv"; // the var name is file
  
  //checks file existence
  if(!filesystem::exists(db_file)){
    cerr << "\033[1;31mERROR!!\033[0m The file " << db_file << " does not exist.\nPlease make sure you have the correct path and that the file is present." << endl;
    exit(1);
  }

  if ( argc == 2 ){
    string_view input = argc > 1 ? argv[1] : "";
    if ( input == "stats" ){ 
      stats = true;  
      defult = false;
    }else if ( input == "history"){ 
      history = true;  
      defult = false;
    }else{
      cerr << "\033[1;31mERROR!!\033[0m Invalid argument: " << input << endl; 
      cerr << "Usage: " << argv[0] << " [stats|history]";
      exit(1);
    }
  }else if ( argc > 2 ){
    cerr << "ERROR! too many arguments.";
    exit(1);
  }else{
    cout << "\nReading file from : " << db_file << endl; 
  }

  vector<Data> store_Data;
  Data entry {};

  // DEFAULT BEHAVIOR
  if ( defult ){
    ifstream in(db_file);
    string output;

    chrono::duration<float> deltaTime; 
   
    vector<string>::iterator ip;
    vector<string> copy_file_names = file_names;

    unordered_map<string, int> delta_t; 
    unordered_map<string, float> scores; 
    unordered_map<string, int> freq_map; 
    unordered_map<string, string> flag_map; 
    unordered_map<string, string> path_map; 
    
    //int line_limit = 0;
    while ( getline(in, output) ){
      istringstream ss( output );
      string output1; // name of the file
      string filepath; // name of the file mhhh yeah 
      tm tm = {}; // struct for the time
      ss >> get_time(&tm,"%a %b %d %H:%M:%S %Y") >> filepath;
      getline( ss >> ws, output1 );
      bool dash = (output1 == "NO_ARG");
      output1 = dash ? filepath + "\tNO_ARG" : filepath;
      flag_map[output1] = dash ? "NO_ARG" : "";
      path_map[output1] = filepath;
      tm.tm_isdst = -1; // takes care of Daylight Saving Time
      file_names.emplace_back( output1 );
      deltaTime = now - chrono::system_clock::from_time_t( mktime(&tm) );
      delta_t[output1]  = chrono::duration_cast<chrono::seconds>( deltaTime ).count(); 
      scores[output1] += weight( delta_t[output1], HL );
      freq_map[output1]++;
      //line_limit++;
    }  
    in.close();
    
    // sort the map by value, I NEED A VECTOR OF PAIRS
    vector<pair<string,float>> ranked( scores.begin(), scores.end() );
    stable_sort( ranked.begin(), ranked.end(), [](auto& a, auto& b){ return a.second > b.second; } );

    for ( auto& f : ranked ){
      entry.cmd = path_map[f.first];
      entry.flag = flag_map[f.first];
      entry.freq = freq_map[f.first];
      entry.score = scores[f.first];
      entry.duration_s = delta_t[f.first];
      store_Data.emplace_back(entry);
    }

  // STATISTCS
  }else if ( stats ){
    read_data(db_file, file_names, deltaTime);

    unordered_map<string, int> score; 
    for ( auto& f : file_names )
      score[f]++; 
    
    // sort the map by value, I NEED A VECTOR OF PAIRS
    vector<pair<string,int>> ranked(score.begin(), score.end());
    stable_sort(ranked.begin(), ranked.end(), [](auto& a, auto& b){ return a.second > b.second; });

    cout << "         NVIM commands statistics:" << endl;
    cout << "\033[1;32mposition, \033[1;34mfrequency, \033[1;31mcmd\033[0m\n";

    int position = 1;
    for ( auto& p : ranked ){
        cout << "\033[1;32m" << position << "." << " \033[1;34m" << p.second  << " \033[1;31m" << p.first << "\033[0m\n";
        position++;
    }
    cout << "\n";
    cout << "Total number of commands: " << file_names.size() <<"\n";
    cout << "Total number of unique commands: " << score.size() <<"\n";

  // PRINTS HISTORY
  }else if ( history ){

    ifstream inh(db_file);
    cout << "          NVIM commands statistics" << endl;
    cout << "\033[1;32m" << "          Date" << "\033[1;34m" << "               Ago" << "\033[1;31m" << "       Name" << "\033[0m\n";
    
    while ( getline(inh, output)){
      istringstream ss(output);
      string output1; // name of the file
      string filepath; // name of the file
      tm tm = {}; // struct for the time
      ss >> get_time(&tm,"%a %b %d %H:%M:%S %Y") >> filepath;
      getline(ss >> ws, output1);
      tm.tm_isdst = -1; // takes care of Daylight Saving Time
      file_names.emplace_back(output1);
      deltaTime = now - chrono::system_clock::from_time_t(mktime(&tm));
      long appo = chrono::duration_cast<chrono::seconds>(deltaTime).count(); 
      cout << "\033[0;32m" << put_time(&tm,"%a %b %d %H:%M:%S %Y") << "  \033[0;34m" << setw(9) << timeAgo(appo) << "  \033[0;31m" << output1 << "\033[0m\n";
    }

    inh.close();
  
  }else{
    cerr << "ERROR: No mode selected. Something went wrong.";
    exit(2);
  }

  int index = pickMenu(store_Data);
  if(index >= 0){
    if (store_Data[index].flag == "NO_ARG"){
      if ( chdir(store_Data[index].cmd.c_str()) == -1 ){ // So the dashboeard opens in another directory, this feeature does not seem to
      // be available in the current version of nvim. NVIM v0.12.6
        cerr << "ERROR: Could not change directory to " << store_Data[index].cmd << endl;
        exit(3);
      }
      chdir(store_Data[index].cmd.c_str());
      execlp("nvim2","nvim2", (char *) nullptr );
      perror( ("execlp nvim2") );
    }
    else{
      execlp("nvim2","nvim2", store_Data[index].cmd.c_str(), (char *) nullptr );
      perror( ("execlp nvim2 " + store_Data[index].cmd) .c_str() );
    }
  }

  return 0;

};

