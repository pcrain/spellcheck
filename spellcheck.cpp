#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <json/json.h>
#include <list>
#include <map>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <regex>

#pragma GCC diagnostic ignored "-fpermissive"
#pragma GCC diagnostic ignored "-pedantic"

#ifndef __linux
   #include <io.h>
   #define access    _access_s
#else
   #include <unistd.h>
#endif

typedef std::map<char,std::list<char> > keymap;
typedef std::map<std::string,int> countmap;
typedef std::list<std::string> stringlist;

static keymap::value_type x[] = {
  std::make_pair('`', std::list<char>{'1','q'}),

  std::make_pair('1', std::list<char>{'2','w','q'}),
  std::make_pair('2', std::list<char>{'1','3','e','w','q'}),
  std::make_pair('3', std::list<char>{'2','4','r','e','w'}),
  std::make_pair('4', std::list<char>{'3','5','t','r','e'}),
  std::make_pair('5', std::list<char>{'4','6','y','t','r'}),
  std::make_pair('6', std::list<char>{'5','7','u','y','t'}),
  std::make_pair('7', std::list<char>{'6','8','i','u','y'}),
  std::make_pair('8', std::list<char>{'7','9','o','i','u'}),
  std::make_pair('9', std::list<char>{'8','0','p','o','i'}),
  std::make_pair('0', std::list<char>{'9','p','o'}),
  std::make_pair('-', std::list<char>{'0','p'}),

  std::make_pair('q', std::list<char>{'1','2','w','s','a'}),
  std::make_pair('w', std::list<char>{'1','2','3','e','d','s','a','q'}),
  std::make_pair('e', std::list<char>{'2','3','4','r','f','d','s','w'}),
  std::make_pair('r', std::list<char>{'3','4','5','t','g','f','d','e'}),
  std::make_pair('t', std::list<char>{'4','5','6','y','h','g','f','r'}),
  std::make_pair('y', std::list<char>{'5','6','7','u','j','h','g','t'}),
  std::make_pair('u', std::list<char>{'6','7','8','i','k','j','h','y'}),
  std::make_pair('i', std::list<char>{'7','8','9','o','l','k','j','u'}),
  std::make_pair('o', std::list<char>{'8','9','0','p','l','k','i'}),
  std::make_pair('p', std::list<char>{'9','0','l','o'}),
  std::make_pair('[', std::list<char>{'0','p'}),

  std::make_pair('a', std::list<char>{'q','w','s','x','z'}),
  std::make_pair('s', std::list<char>{'q','w','e','d','c','x','z','a'}),
  std::make_pair('d', std::list<char>{'w','e','r','f','v','c','x','s'}),
  std::make_pair('f', std::list<char>{'e','r','t','g','b','v','c','d'}),
  std::make_pair('g', std::list<char>{'r','t','y','h','n','b','v','f'}),
  std::make_pair('h', std::list<char>{'t','y','u','j','m','n','b','g'}),
  std::make_pair('j', std::list<char>{'y','u','i','k','m','n','h'}),
  std::make_pair('k', std::list<char>{'u','i','o','l','m','j'}),
  std::make_pair('l', std::list<char>{'i','o','p','k'}),
  std::make_pair(';', std::list<char>{'o','p','l'}),
  std::make_pair('\'', std::list<char>{'p'}),

  std::make_pair('z', std::list<char>{'a','s','x'}),
  std::make_pair('x', std::list<char>{'a','s','d','c','z'}),
  std::make_pair('c', std::list<char>{'s','d','f','v','x'}),
  std::make_pair('v', std::list<char>{'d','f','g','b','c'}),
  std::make_pair('b', std::list<char>{'f','g','h','n','v'}),
  std::make_pair('n', std::list<char>{'g','h','j','m','b'}),
  std::make_pair('m', std::list<char>{'h','j','k','n'}),
  std::make_pair(',', std::list<char>{'j','k','l','m'}),
  std::make_pair('.', std::list<char>{'k','l'}),
  std::make_pair('/', std::list<char>{'l'}),
};
static keymap KEYS(x, x + sizeof x / sizeof x[0]);


const int MATCHES = 1;

const char* RED = "\033[1;31m";
const char* GRN = "\033[1;32m";
const char* YLW = "\033[1;33m";
const char* BLU = "\033[1;34m";
const char* MGN = "\033[1;35m";
const char* CYN = "\033[1;36m";
const char* WHT = "\033[1;37m";
const char* BLN = "\033[0m";

const std::string OUTFILE = "wikiout";
const char DELIMITER = ' ';

const std::regex word("[a-zA-Z']+(-[a-zA-Z']+)?");

static std::map<std::string,int> wordcounts;
static stringlist firstletters[27][27][50];

struct replacement{
  std::string s;
  int v;
};

static replacement reps[10];

//Globals

bool training = false;
bool debugcost = false;
bool toterminal = false;
bool continuous = false;

float delweight = 3;
float delscale = 2;
float subweight = 2;
float subscale = 2;
float insweight = 0.5f;
float insscale = 2;

int nthreads = 1;
int ldelta = 2;
int wcthreshold = 1;

std::string dictfile = "/home/pretzel/workspace/spellcheck/alldicts";
std::string trainfile = "/home/pretzel/data/gutenberg/allpaths";

bool FileExists(const std::string &Filename ) {
    return access( Filename.c_str(), 0 ) == 0;
}

void LoadDict(std::string dpath) {
  if (! FileExists(dpath)) {
    std::cout << "Dictionary " << dpath << " does not exist" << std::endl;
    return;
  }
  // return;
  std::ifstream input(dpath);
  Json::Value root;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(input, root);
  if (parsingSuccessful ) {
    for(Json::ValueIterator itr = root.begin() ; itr != root.end() ; itr++ ) {
      // std::cout << itr.key() << ": " << *itr << "\n";
      std::string s = itr.key().asString();
      int sl = s.length()-1;
      int wc = (*itr).asInt();
      if (wc < wcthreshold)
        continue;
      wordcounts[s] = wc;
      // std::cout << s << ":" << wordcounts[s] << std::endl;
      if (s[0] != '\'') {
        if (s[sl] != '\'') {
          firstletters[s[0]-97][s[sl]-97][sl].push_back(s);
        } else {
          firstletters[s[0]-97][26][sl].push_back(s);
        }
      } else {
        if (s[sl] != '\'') {
          firstletters[26][s[sl]-97][sl].push_back(s);
        } else {
          firstletters[26][26][sl].push_back(s);
        }
      }
    }
    input.close();
    return;
  }
  std::cout  << "Failed to parse configuration\n";
  input.close();
}

void LoadConfig(std::string dpath) {
  if (! FileExists(dpath)) {
    std::cout << "Config file " << dpath << " does not exist" << std::endl;
    return;
  }
  // return;
  std::ifstream input(dpath);
  Json::Value root;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(input, root);
  if (parsingSuccessful ) {
    for(Json::ValueIterator itr = root.begin() ; itr != root.end() ; itr++ ) {
      std::string s = itr.key().asString();
      // wordcounts[s] = (*itr).asInt();
      if (s.compare("dictfile") == 0) {
        dictfile = (*itr).asString();
        // std::cout << "Using dictionary " << dictfile << std::endl;
        continue;
      }
      if (s.compare("ldelta") == 0) {
        ldelta = (*itr).asInt();
        // std::cout << "Using ldeltas of " << std::to_string(ldelta) << std::endl;
        continue;
      }
      if (s.compare("wcthreshold") == 0) {
        wcthreshold = (*itr).asInt();
        // std::cout << "Using wcthreshold of " << std::to_string(wcthreshold) << std::endl;
        continue;
      }
    }
    input.close();
    return;
  }
  std::cout  << "Failed to parse configuration\n";
  input.close();
}

int LevenPrint(std::string s, std::string t, int** d, bool keydist) {
  int weight = 0;

  int i = s.length(), ls = i + 1;
  int j = t.length(), lt = j + 1;

  //Print the changes
  std::string temp = s;
  // std::cout << temp << std::endl;
  float inserts = insweight, subs = subweight, dels = delweight;
  while (i > 0 || j > 0) {
    int c = d[i][j];
    if (i > 0 && j > 0 && d[i-1][j-1] <= d[i-1][j] && d[i-1][j-1] <= d[i][j-1] && d[i-1][j-1] <= c) {
      i -= 1; j -= 1;
      if (d[i][j] < c) {  //Substitute
        if (keydist) {
          char ikey = temp[i];
          // std::cout << ikey << " -> " << jkey;
          if (KEYS.find(ikey) != KEYS.end()) {
            if (std::find(KEYS[ikey].begin(),KEYS[ikey].end(),t[j]) != KEYS[ikey].end()) {
              weight += 1;
              // std::cout << GRN << "Neighbors" << BLN;
            } else {
              weight -= 1;
              // std::cout << RED << "Not Neighbors" << BLN;
            }
          }
          // std::cout << std::endl;
        }
        weight -= subs;
        subs*=subscale;
        // std::cout << temp.substr(0,i) << YLW << t[j] << BLN << temp.substr(i+1,temp.length()-i-1);
        // temp = temp.substr(0,i)+t[j]+temp.substr(i+1,temp.length()-i-1);
      } else continue;
    }
    else if (j > 0 && (i == 0 || (d[i][j-1] <= d[i-1][j])) && d[i][j-1] <= c) { //Insert
      j -= 1;
      // std::cout << temp.substr(0,i) << GRN << t[j] << BLN << temp.substr(i,temp.length()-i);
      // temp = temp.substr(0,i)+t[j]+temp.substr(i,temp.length()-i);
      weight -= inserts;
      inserts*=insscale;
    }
    else if (i > 0) { //Delete
      i -= 1;
      // std::cout << temp.substr(0,i) << RED << temp[i] << BLN << temp.substr(i+1,temp.length()-i-1);
      // temp = temp.substr(0,i)+temp.substr(i+1,temp.length()-i-1);
      weight -= dels;
      dels*=delscale;
    }
    else {
      j -= 1; continue; //Do nothing
    }
    // std::cout << std::endl;
  }
  // std::cout << temp << std::endl;
  return weight;
}

int LevenI(std::string s, std::string t, bool outmatrix, int &keydist) {
  int ls = s.length()+1;
  int lt = t.length()+1;

  int** d = new int*[ls];
  for (int i = 0; i < ls; ++i) {
    d[i] = new int[lt];
    for (int j = 0; j < lt; ++j) {
      d[i][j] = 0;
    }
  }

  for (int i = 0; i < ls; ++i) {
    if (i == 0) {
      for (int j = 1; j < lt; ++j) {
        d[0][j] = j;
      }
    }
    else {
      d[i][0] = i;
      for (int j = 1; j < lt; ++j) {
        d[i][j] = 0;
      }
    }
  }

  for (int j = 1; j < lt; ++j) {
    for (int i = 1; i < ls; ++i) {
      int a = d[i-1][j] + 1;
      int b = d[i][j-1] + 1;
      if (b < a)
        a = b;
      int c = d[i-1][j-1] + ((s[i-1] == t[j-1]) ? 0 : 1);
      d[i][j] = ((a < c) ? a : c);
    }
  }

  int r = d[ls-1][lt-1];
  int kd = LevenPrint(s,t,d,keydist);
  keydist = r - kd;
  for (int i = 0; i < ls; ++i)
    delete d[i];
  delete [] d;
  return r;
}

int Tokenize(std::string str/*, std::vector<std::string> &token_v*/){
    std::smatch m;
    size_t start = str.find_first_not_of(DELIMITER), end=start;
    std::string s;
    while (start != std::string::npos){
        end = str.find(DELIMITER, start);
        // token_v.push_back(str.substr(start, end-start));
        s = str.substr(start, end-start);
        if(s.length() == 0 || ! std::regex_search(s,m,word)) {
          return 0;
        }
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        while (std::regex_search (s,m,word)) {
          // for (auto x:m) std::cout << MGN << x << BLN << " ";
          // std::cout << m[0] << std::endl;
          if (m[0] == "verantwortungsfreudigkeit") {
            std::cout << YLW << "GERMAN" << BLN << std::endl;
            return -1;
          }
          if (m[0] == "autres" || m[0] == "merveilles") {
            std::cout << YLW << "FRENCH" << BLN << std::endl;
            return -1;
          }
          if (m[0] == "poder") {
            std::cout << YLW << "SPANISH" << BLN << std::endl;
            return -1;
          }
          if (m[0] == "zonder") {
            std::cout << YLW << "DUTCH" << BLN << std::endl;
            return -1;
          }
          if (m[0] == "conueniebat" || m[0] == "mareuil") {
            std::cout << MGN << "WHAT" << BLN << std::endl;
            return -1;
          }
          if (wordcounts.count(m[0]) == 0) {
            wordcounts[m[0]] = 1;
          } else {
            wordcounts[m[0]] += 1;
          }
          s = m.suffix().str();
        }
        // std::cout << MGN << s << BLN << std::endl;
        start = str.find_first_not_of(DELIMITER, end);
    }
    return 0;
}

int ReadFile(std::string fname) {
  bool found = false;

  std::ifstream file(fname);
  std::string str;
  // std::regex startline("^\\ *\\*\\*\\*[^\\*]+\\*\\*\\*\\ *$");
  std::smatch m;
  std::regex startline("START");
  while (std::getline(file, str)) {
    // std::cout << str << std::endl;
    if(std::regex_search(str,m,startline)) {
      // std::cout << GRN << "GOOD" << BLN << std::endl;
      std::cout << GRN << str << BLN << std::endl;
      found = true;
      break;
    }
  }
  if (!found) {
      std::cout << RED << "BAD" << BLN << std::endl;
      return -1;
  }

  // std::regex endline("\\*\\*\\*\\ *END");
  std::regex endline("^\\*\\*\\*\\ *[Ee][Nn][Dd]");
  found = false;
  while (std::getline(file, str)) {
    // std::cout << str << std::endl;
    if(std::regex_search(str,m,endline)) {
      // std::cout << GRN << "GOOD" << BLN << std::endl;
      std::cout << CYN << str << BLN << std::endl;
      found = true;
      break;
    } else {
      if (Tokenize(str) < 0)
        return -2;  //Foreign language
      //Process line
    }
  }
  if (!found) {
      std::cout << YLW << "BAD" << BLN << std::endl;
      return -1;
  }
  return 0;
}

std::vector<std::string> Splitstring(std::string s, char delim) {
  std::stringstream test(s);
  std::string segment;
  std::vector<std::string> seglist;
  while(std::getline(test, segment, delim)) {
     seglist.push_back(segment);
  }
  return seglist;
}

int ReadFileList(std::string fname) {
  std::ifstream file(fname);
  std::string str;
  while (std::getline(file, str)) {
    std::string basename = Splitstring(str,'/').back();
    std::string outfname = "_temp-dictionaries/dictionary-"+basename;
    if (FileExists(outfname)) {
      std::cout << "File " << str << " already scanned\n";
      continue;
    }
    std::cout << str << std::endl;
    if (ReadFile(str) < 0) {
      wordcounts.clear();
      continue;
    }
    std::ofstream fout(outfname);
    fout << "{\n";
    bool first = true;
    for ( const auto &p : wordcounts ) {
      if (first) {
        first = false;
      } else {
        fout << ",\n";
      }
      fout << "  \"" << p.first << "\" : " << p.second;
    }
    fout << "\n}\n";
    fout.close();
    std::cout << outfname << std::endl;

    // break;
    wordcounts.clear();
  }
  file.close();
}

std::string FindReplacements(std::string s, std::fstream &fs) {
  int sl = s.length();
  int ind[2], lind[2];
  for (int i = 0; i < 2; ++i) {
    if (s[i] == '-')
      ind[i] = -1;
    else
      ind[i] = (s[i] == '\'') ? 26 : (s[i]-97);
    if (s[sl-i-1] == '-')
      lind[i] = -1;
    else
      lind[i] = (s[sl-i-1] == '\'') ? 26 : (s[sl-i-1]-97);
  }

  for (int i = 0; i < MATCHES; ++i)
    reps[i] = {"",99};

  //Check for single letter swap
  std::string bs = "";
  int bsc = 99;
  for(int i = 1; i < sl; ++i) {
    std::string swap =
      s.substr(0,i-1) + s[i] + s[i-1];
    if (i < sl-1)
      swap = swap + s.substr(i+1,sl-(i+1));
    if (wordcounts.count(swap) > 0) {
      if (wordcounts[swap] > bsc) {
        bs = swap;
        bsc = wordcounts[swap];
      }
    }
  }
  if (bs.compare("") != 0) {
    if (debugcost)
      std::cout << GRN << "  " << bs << ": SWAP" << BLN << "\n";
    return bs;
  }

  int min = ((sl <= ldelta) ? 0 : sl-ldelta-1);
  int max = sl+ldelta;
  #pragma omp parallel for num_threads(nthreads)
  for (int j = 0; j < 4; ++j) {
    if (lind[j/2] == -1)
      continue;
    if (ind[j%2] == -1)
      continue;
    for (int m = min; m < max; ++m) {
      for (stringlist::const_iterator it = firstletters[ind[j/2]][lind[j%2]][m].begin(); it != firstletters[ind[j/2]][lind[j%2]][m].end(); ++it) {
        std::string right = (*it);
        int l = abs(right.length()-sl);
        int kd;
        int d = LevenI(s,right,false,kd);
        // std::cout << k << ",";
        int c = 8-log10(wordcounts[right]);
        if (c < 1)
          c = 1;
        // std::cout << c << ",";
        int sum = l+d+kd+c;
        // std::cout << sum << "\n";
        if (MATCHES>1) {
          if (reps[MATCHES-1].v > sum) {
            #pragma omp critical
            {
              if (reps[MATCHES-1].v > sum) {
                int rank;
                for (rank = 0; rank < MATCHES; ++rank) {
                  if (reps[rank].v > sum)
                    break;
                }
                for (int i = MATCHES-2; i >= rank; --i) {
                  reps[i+1] = reps[i];
                }
                reps[rank] = {right,sum};
              }
            }
          }
        } else {
          if (reps[0].v >= sum) {
            #pragma omp critical
            {
              if (reps[0].v > sum || (reps[0].v == sum && wordcounts[right] > wordcounts[reps[0].s]))
                reps[0] = {right,sum};
            }
          }
        }
      }
    }
  }
  if (debugcost) {
    std::cout << GRN << "  " << reps[0].s << ": " << reps[0].v << BLN << "\n";
    for (int i = 1; i < MATCHES; ++i) {
      std::cout << "  " << reps[i].s << ": " << reps[i].v << "\n";
    }
  }
  return reps[0].s;
}

void CheckFile(std::string fname) {

  std::fstream fs;
  remove(std::string(fname+"out").c_str());
  fs.open (fname+"out", std::fstream::out | std::fstream::app);

  std::ifstream infile(fname);
  std::string str;
  while (std::getline(infile, str)) {
    std::smatch m;
    size_t start = str.find_first_not_of(DELIMITER), end=start;
    std::string s;
    bool first = true;
    while (start != std::string::npos){
        std::string startpunc = "", endpunc = "";
        bool haspunc = false;
        end = str.find(DELIMITER, start);
        s = str.substr(start, end-start);
        int sl = s.length();
        // std::cout << s << std::endl;
        if((s[0] == '\'' && s[sl-1] == '\'') || !std::regex_match(s,word)) {
          //Strip punctuations
          for (int i = 0; i < sl; ++i) {
            if ( (s[i] > 64 && s[i] < 91) || (s[i] > 96 && s[i] < 123) ) {
              startpunc = s.substr(0,i);
              s = s.substr(i,sl-i);
              break;
            }
          }
          sl = s.length();
          for (int i = sl-1; i > 0; --i) {
            if ( (s[i] > 64 && s[i] < 91) || (s[i] > 96 && s[i] < 123) ) {
              endpunc = s.substr(i+1,sl-i);
              s = s.substr(0,i+1);
              break;
            }
          }
          // std::cout << startpunc << MGN << s << BLN << endpunc << std::endl;

          if(!std::regex_match(s,word)) {
            // std::cout << YLW << s << BLN << " cannot be checked\n";
            if (first) {
              if (toterminal)
                std::cout << startpunc << s << endpunc;
              fs << startpunc << s << endpunc;
              first = false;
            }
            else {
              if (toterminal)
                std::cout << DELIMITER << startpunc << s << endpunc;
              fs << DELIMITER << startpunc << s << endpunc;
            }
            start = str.find_first_not_of(DELIMITER, end);
            continue;
          }
          haspunc = true;
        }
        bool caps = false;
        if (s[0] > 64 && s[0] < 91) {
          if (s[sl-1] > 64 && s[sl-1] < 91) {
            // std::cout << CYN << s << BLN << " is an acronym\n";
            if (first) {
              if (toterminal)
                std::cout << s;
              fs << s;
              first = false;
            }
            else {
              if (toterminal)
                std::cout << DELIMITER << s;
              fs << DELIMITER << s;
            }
            if (haspunc) {
              if (toterminal)
                std::cout << endpunc;
              fs << endpunc;
            }
            start = str.find_first_not_of(DELIMITER, end);
            continue;
          }
          caps = true;
          std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        }
        if (wordcounts.count(s) == 0) {
          if (s.length() < 2) {
            //1 letter word
            if (first) {
              if (toterminal)
                std::cout << startpunc << s << endpunc;
              fs << startpunc << s << endpunc;
              first = false;
            }
            else {
              if (toterminal)
                std::cout << DELIMITER << startpunc << s << endpunc;
              fs << DELIMITER << startpunc << s << endpunc;
            }
            start = str.find_first_not_of(DELIMITER, end);
            continue;
          }
          if (debugcost)
            std::cout <<
              str.substr(0,start) <<
              RED <<
              str.substr(start, end-start) <<
              BLN <<
              ((str.length() >= end) ? str.substr(end,str.length()-end) : "") <<
              std::endl;
          // std::cout << RED << m[0] << BLN << " is not a word\n";
          std::string best = FindReplacements(s,fs);
          if (caps && s[0] != '\'')
            best[0] -= 32;  //To uppercase
          if (first) {
            if (haspunc) {
              if (toterminal)
                std::cout << startpunc;
              fs << startpunc;
            }
            if (toterminal)
              std::cout << RED << str.substr(start, end-start) << GRN << best << BLN;
            fs << best;
            first = false;
          }
          else {
            if (toterminal)
              std::cout << DELIMITER;
            fs << DELIMITER;
            if (haspunc) {
              if (toterminal)
                std::cout << startpunc;
              fs << startpunc;
            }
            if (toterminal)
              std::cout << RED << str.substr(start, end-start) << GRN << best << BLN;
            fs << best;
          }
        } else {
          // std::cout << m[0] <<" is a word\n";
          if (caps && s[0] != '\'')
            s[0] -= 32;  //To uppercase
          if (first) {
            if (haspunc) {
              if (toterminal)
                std::cout << startpunc;
              fs << startpunc;
            }
            if (toterminal)
              std::cout << s;
            fs << s;
            first = false;
          }
          else {
            if (toterminal)
              std::cout << DELIMITER;
            fs << DELIMITER;
            if (haspunc) {
              if (toterminal)
                std::cout << startpunc;
              fs << startpunc;
            }
            if (toterminal)
              std::cout << s;
            fs << s;
          }
        }
        if (haspunc) {
          if (toterminal)
            std::cout << endpunc;
          fs << endpunc;
        }
        // std::cout << MGN << s << BLN << std::endl;
        start = str.find_first_not_of(DELIMITER, end);
    }
    if (toterminal)
      std::cout << "\n";
     fs << "\n";
  }
  infile.close();
  fs.close();
}

int ParseArgs(int argc, char** argv) {
  int opt;
  // Shut GetOpt error messages down (return '?'):
  opterr = 0;
  // Retrieve the options:
  while ( (opt = getopt(argc, argv, ":ci:s:d:t:vxm:")) != -1 ) {  // for each option...
    switch ( opt ) {
      case 'c':
        continuous = true;
        break;
      case 'i':
        insweight = std::atof(optarg);
        std::cout << "Setting insweight to " << std::to_string(insweight) << std::endl;
        break;
      case 's':
        subweight = std::atof(optarg);
        std::cout << "Setting subweight to " << std::to_string(subweight) << std::endl;
        break;
      case 'd':
        delweight = std::atof(optarg);
        std::cout << "Setting delweight to " << std::to_string(delweight) << std::endl;
        break;
      case 't':
        training = true;
        trainfile = std::string(optarg);
        std::cout << "Training with files listed in " << trainfile << std::endl;
        break;
      case 'v':
        debugcost = true;
        break;
      case 'x':
        toterminal = true;
        break;
      case 'm':
        nthreads = std::atoi(optarg);
        std::cout << "Using " << std::to_string(nthreads) << " threads" << std::endl;
      case '?':  // unknown option...
        std::cerr << "Unknown option: '" << char(optopt) << "'!" << std::endl;
        break;
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  LoadConfig("/home/pretzel/workspace/spellcheck/sc-config.json");
  ParseArgs(argc,argv);
  if (training) {
    ReadFileList(trainfile);
  } else if (continuous || optind < argc) {
    LoadDict(dictfile);
    if (continuous) {
      std::string file;
      while (true) {
        std::cout << "Enter name of file to check (q to quit): ";
        std::cin >> file;
        if (file.compare("q") == 0)
          break;
        if (FileExists(file))
          CheckFile(file);
        else
          std::cout << "File " << file << " does not exist" << std::endl;
      }
    } else {
      CheckFile(argv[optind]);
    }
  } else {
    std::cout << "No file to check specified" << std::endl;
  }
  return 0;
}
