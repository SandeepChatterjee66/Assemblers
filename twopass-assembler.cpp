#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <iomanip>

using namespace std;

void init();
void input(string file="");
void output();
void pass1();
void pass2();


int main(int argc, char *argv[]){
  init();
  if(argc==2) input(argv[1]);
  else        input();
  pass1();
  pass2();
  std::cout<<std::endl;
  return 0;
}

struct mnemonic{//MOT Table format
  std::string name;
  int    size;
  std::string opcode;
  std::string format;
}mot[25];

struct psuedocode{//POT Table format
	std::string name;
	int    size;
}pot[9];

struct symbol{ //Symbol Table format
	std::string name;
	std::string type;
	int location;
	int size;
	int section_id;
	std::string is_global;
};

struct section{ //Section Table format
	int id;
	std::string name;
	int size;
  int loc;
};


std::string toString(int i);
std::string toHex(int i);
std::string toHex(int i, int bytes);
std::string toBin(std::string hex);
bool replace(std::string& str,
     const std::string& from, const std::string& to);

int c  = 0; //line number of parsing
int lc = 0; //Controls Location Counter
int sec_id = 0; //Manage section Id
int var_lc; //Store location of variable in Pass2
std::ifstream infile; //Input File Stream
std::ofstream inpfile; //Output file stream
std::ofstream outfile; //Output File Stream
std::ofstream objhexfile;
std::ofstream objbinfile;
std::string word; //Read Word by Word from file
std::string temp; //Temporary Variable
mnemonic control;
int size;  //Control Variable for search
unordered_map<std::string, mnemonic  > MOT;
unordered_map<std::string, psuedocode> POT;
vector<symbol> symlab; //Symbol Table
vector<section> sec; //Section Table



void init()
{
	//Initializing Machine Opcode Table
  //         Mnemonic Size Opcode OperandFormat
  mot[ 0] = {"MVI A",  5, "00", "{R,C}"};
	mot[ 1] = {"MVI B",  5, "01", "{R,C}"};
	mot[ 2] = {"MVI C",  5, "02", "{R,C}"};
	mot[ 3] = {"MVI I",  5, "03", "{R,C}"};
	mot[ 4] = {"LOAD",   5, "04", "{C}"  };
	mot[ 5] = {"STORE",  5, "05", "{C}"  };
	mot[ 6] = {"LOADI",  1, "06", "{}"   };
	mot[ 7] = {"STORI",  1, "07", "{}"   };
	mot[ 8] = {"ADD B",  1, "08", "{R}"  };
	mot[ 9] = {"ADD C",  1, "09", "{R}"  };
	mot[10] = {"MOV A,B",1, "0A", "{R,R}"};
	mot[11] = {"MOV A,C",1, "0B", "{R,R}"};
	mot[12] = {"MOV B,C",1, "0C", "{R,R}"};
	mot[13] = {"MOV B,A",1, "0D", "{R,R}"};
	mot[14] = {"MOV C,A",1, "0E", "{R,R}"};
	mot[15] = {"MOV C,B",1, "0F", "{R,R}"};
	mot[16] = {"INC A",  1, "10", "{R}"  };
	mot[17] = {"INC B",  1, "11", "{R}"  };
	mot[18] = {"INC C",  1, "12", "{R}"  };
	mot[19] = {"CMP A",  1, "13", "{R}"  };
	mot[20] = {"CMP B",  1, "14", "{R}"  };
	mot[21] = {"CMP C",  1, "15", "{R}"  };
	mot[22] = {"ADDI",   5, "16", "{C}"  };
	mot[23] = {"JE",     5, "17", "{C}"  };
	mot[24] = {"STOP",   1, "18", "{}"   };

	//Initializing pot Table
	pot[0] = {"dw",   2};//Define Word
	pot[1] = {"dd",   4};//Define Double word
	pot[2] = {"dq",   8};//Define Double Precision float
	pot[3] = {"dt",   9};//Define extended precision float
	pot[4] = {"resb", 1};//Reserve byte
	pot[5] = {"resw", 2};//Reserve word
	pot[6] = {"resd", 8};//Reserve double word
	pot[7] = {"resq", 8};//Reserve double Precision
	pot[8] = {"rest", 9};//Reserve extended Precision

  for(int i=0; i<25; i++)
    MOT[mot[i].name]=mot[i];

  for(int i=0; i<9; i++)
   POT[pot[i].name]=pot[i];
}

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

int search_symbol(string variable) //Find Location of the Given Symbol
{
	int location = -1;
	for(vector<symbol>::const_iterator i = symlab.begin();i != symlab.end();++i)
	{
		if(i->name == variable)
		{
			location = i->location;
			break;
		}
	}
  	return location;
}

int size_evaluation(string data, int bytes=4) //Evaluate size of Variable defined
{
	int size = 0;
	for(int i = 0;i < data.length();i++)
	{
		if(data[i] == ',' || data[i] == ' ' || data[i] == '\n')
			size += bytes;
	}
	size += bytes;
	return size;
}


string data_break(string data, int bytes) //Convert String of Input Number into Binary String
{
	string final="";
	string temporary = "";
	for(int i = 0;i < data.length();i++)
	{
		if(data[i] == ',' || data[i] == ' ' || data[i] == '\n')
		{
			final = final+toHex(atoi(temporary.c_str()), bytes)+" ";
			temporary = "";
		}
		else
			temporary += data[i];
	}

	return final;
}

int decode_register(string word){
  if(word[0]=='A')
   return 1;
  else if(word[0]=='B')
   return 2;
  else if(word[0] == 'C')
   return 3;
  else if(word[0] == 'I')
   return 4;
  return -1;
}

void store_symlab() //Storing Symbol Table in File
{
  cout<<"\n\nSYMBOL TABLE\n-----------------------"<<endl;
	outfile.open("symbol.csv");
	outfile << "Name,Type,Location,Size,SectionID,IsGlobal\n";
  cout<<setw(8)<<"Name "<<setw(8)<<"Type"<<setw(9)<<"   Location"<<setw(4)<<"   Size "<<setw(10)<<" SectionID";
  cout<<setw(8)<<"  IsGlobal"<<"\n";
	for(vector<symbol>::const_iterator i = symlab.begin();i != symlab.end();++i)
	{
		outfile << i->name<<",";       cout <<setw(8)<< i->name<<" ";
		outfile << i->type<<",";       cout <<setw(8)<< i->type<<" ";
		outfile << i->location<<",";
            if(i->location == -1)  cout <<setw(9)<< "---"   <<" ";
            else                   cout <<setw(9)<< i->location<<" ";
		outfile << i->size<<",";
            if(i->size == -1)      cout<<setw(4)<< "---"    <<" ";
            else                   cout<<setw(4)<<i->size   <<" ";
		outfile << i->section_id<<",";
            if(i->section_id == -1)cout<<setw(10)<< "---"        <<" ";
            else                   cout<<setw(10)<<i->section_id <<" ";
		outfile << i->is_global<<"\n"; cout <<setw(8)<< i->is_global<<"\n";
	}
	outfile.close();
}

void store_sec() //Storing Section Table in File
{
  cout<<"\n\nSECTION TABLE\n-----------------------\n";
	outfile.open("section.csv");
	outfile << "ID,Name,Size\n";
  cout<<setw(4)<<"ID"<<setw(8)<<"Name"<<setw(5)<<"Size"<<setw(5)<<"    Pointer to content\n";
	for(vector<section>::const_iterator i = sec.begin();i != sec.end();++i)
	{
		outfile << i->id  <<",";     cout<<setw(4)<<i->id  <<" ";
		outfile << i->name<<",";     cout<<setw(8)<<i->name<<" ";
		outfile << i->size<<",";     cout<<setw(5)<<i->size<<" ";
    outfile << i->loc <<"\n";    cout<<setw(5)<<i->loc <<"\n";
	}
	outfile.close();
}

void display_file(string filename){
  ifstream inputfile;
  string line;
  int n=0;
  cout<<"\n\nCONTENTS ARE : \n";
  inputfile.open(filename);
  while(getline(inputfile,line))
     cout <<setw(3)<<++n<<"    "<< line << "\n";
  inputfile.close();
}


void input(string file){
  if(file!="")
  cout<<"--------------------------------------------------------\n";
  cout<<"                WELCOME TO MY_ASSEMBLER\n";
  cout<<"--------------------------------------------------------\n";
  cout<<"Enter the source code input in assembly:\n";
  cout<<"\t1.Enter in the console\n";
  cout<<"\t2.Input from the text file\n";

  char ch;
  cin>>ch;

  if(ch=='1'){
      inpfile.open("input.txt");
      while(ch!='~') {
        ch = cin.get();
        if(ch!='~') inpfile<<ch;
      }
      inpfile.close();
  }else{
    //ch is 2
    int n=0;
    string line;
    if(file==""){
      cout<<"Enter the filename: ";
      cin>>line;
    }else{
      line = file;
    }
    ifstream inputfile;
    inputfile.open(line);
    inpfile.open("input.txt");
    cout<<"Contents of the file are here :\n";
    while(getline(inputfile,line)){
       inpfile << line << "\n";
       cout <<setw(3)<<++n<<"    "<< line << "\n";
    }
    cout<<"\n\n\n";
    inputfile.close();
    inpfile.close();
  }

}



void pass1(){
  std::ifstream file;
  std::string str;
  file.open("input.txt");
  while (std::getline(file, str)) {
    if(str=="")           //ignore blank line
     continue;
    str = str.substr(0, str.find(';')); //ignore comments
    replace(str, ", ", ","); //remove space after comma
    std::stringstream line(str);
    std::string word;
    line>>word;

    if(word.find(":") != -1)//Declaration of Label is Found
    {
      //cout<<"label found "<<word<<endl;
      symlab.push_back({word.erase(word.length()-1,1),"label",lc,-1,sec_id,"false"}); //Inserting into Symbol Table
      line>>word;
    }

    temp = word;
      //std::cout<<word<<"- ";
      if(word == "section")//Section is Found
  			{
  				line >> word;
  				sec_id++;
  				sec.push_back({sec_id,word,0, c}); //Inserting into Section Table
  				if(sec_id != 1) // Updating previous section Size
  				{
  					sec[sec_id-2].size = lc;
  					lc = 0;
  				}
          if(line>>word)
           std::cout<<"Error: unexpected identifier after section declaration, at line "<<c+1<<"\n"<<str;
  			}
  			else if(word == "global") //Global Varaible is Found
  			{
  				line >> word;
  				symlab.push_back({word,"label",-1,-1,-1,"true"}); //Inserting into Symbol Table
          if(line>>word)
            std::cout<<"Error: unexpected identifier after extern declaration, at line "<<c+1<<"\n"<<str;
  			}
  			else if(word == "extern") //External Variable is found
  			{
  				line >> word;
  				symlab.push_back({word,"external",-1,-1,-1,"false"}); //Inserting into Symbol Table
          if(line>>word)
            std::cout<<"Error: unexpected identifier after section declaration, at line "<<c+1<<"\n"<<str;
  			}
        else if(MOT.find(word)!=MOT.end()){
            //cout<<word<<"found in MOT 1 "<<endl;
            control = MOT[word];

      			if(!(control.format=="{}" && control.name!="STOP"))
      				line >> word;

      			if(control.format=="{R,C}" || control.format=="{R,R}")
      				line >> word;
      			lc += control.size;
            c  += control.size;
        }
        else
        {
              line>>word;
              //cout<<temp<<" "<<word[0]<<"found in MOT 2"<<endl;
              if(MOT.find(temp+" "+word[0])!=MOT.end()){
                  control = MOT[temp+" "+word[0]];

            			if(!(control.format=="{}" && control.name!="STOP"))
            				line >> word;

            			if(control.format=="{R,C}" || control.format=="{R,R}")
            				line >> word;
            			lc += control.size;
                  c  += control.size;
              }
              else if(MOT.find(temp+" "+word)!=MOT.end()){
                  control = MOT[temp+" "+word];

            			if(!(control.format=="{}" && control.name!="STOP"))
            				line >> word;

            			if(control.format=="{R,C}" || control.format=="{R,R}")
            				line >> word;
            			lc += control.size;
                  c  += control.size;
              }
             else{
                  int bytes = 4;
                  if(POT.find(word)!=POT.end())
                    bytes = POT[word].size;
                  //Variable is Found
          				line >> word;
                  size = size_evaluation(word, bytes);
                  var_lc = search_symbol(temp);
          				if(var_lc != -1){
                    cout<<"redefination of already defined symbol, line "<<c+1<<"\n"<<str;
                    return;
                  }
                  //cout<<temp<<"not found in MOT 3 jai gau mata"<<endl;
          				symlab.push_back({temp,"var",lc,size,sec_id,"false"}); //Inserting into Symbol Table
          				lc += size;
                  c  += size;
          			}
            }

    //std::cout<<std::endl;
  }
  sec[sec_id-1].size = lc; //Updating size of current Section

  store_symlab();
  store_sec();
  file.close();
}

void pass2(){
  infile.open("input.txt");
  string str;
  objhexfile.open("outputhex.txt");
  while(std::getline(infile, str)){
    if(str=="")           //ignore blank line
     continue;
    str = str.substr(0, str.find(';')); //ignore comments
    replace(str, ", ", ","); //remove space after comma
    std::stringstream line(str);
    std::string word;
    line>>word;
    temp = word;

    if(word.find(":") != -1 ||
                word == "global"|| word == "extern"){
                  continue;
                  c++;
                }
    if(word == "section") //No change in Section content
    {
        infile >> word;
        lc = 0;
        c++;
        continue;
    }

    mnemonic control;

    if(MOT.find(word)!=MOT.end()){
        control = MOT[word];
        cout <<toHex(lc, 2)       <<"| "<<control.opcode;
        objhexfile <<control.opcode <<" ";
        lc += control.size;
        c  += control.size;

        if(control.format=="{C}"){
          line>>word;
          string operand = word;
          var_lc = search_symbol(operand);
          if(var_lc == -1){
            if(is_number(operand)){
              objhexfile <<toHex(atoi(operand.c_str()), 2)<<" ";
                    cout <<toHex(atoi(operand.c_str()), 2)<<" ";
            }else{
              cout<<"Error, symbol "<<operand<<" not found in symbol table "<<endl;
            }
          }
          else{
            objhexfile <<" "<<toHex(var_lc, 2);
            objbinfile <<" "<<toBin(toHex(var_lc, 2));
          }
          if(line>>word)
            std::cout<<"Error: unexpected identifier "<<word<<" after operand, at line "<<c+1<<"\n"<<str;
        }
        else if(control.format=="{}"){
          if(line>>word)
            std::cout<<"Error: unexpected identifier "<<word<<" after opcode, at line "<<c+1<<"\n"<<str;
        }
    }
    else
    {
        line>>word;
        if(MOT.find(temp+" "+word)!=MOT.end()){
          control = MOT[temp+" "+word];
                cout <<toHex(lc, 2)       <<"| "<<control.opcode;
          objhexfile <<control.opcode <<" ";
          lc += control.size;

          if(control.format=="{R,C}"){
            string operand = word.substr(str.find(',')+1, str.size());
            var_lc = search_symbol(operand);
            if(var_lc == -1){
              if(is_number(operand)){
                objhexfile <<toHex(atoi(operand.c_str()), 2)<<" ";
                      cout <<toHex(atoi(operand.c_str()), 2)<<" ";
              }else{
                cout<<"Error, symbol "<<operand<<" not found in symbol table "<<endl;
              }
            }
            else if(control.format=="{R,R}" || control.format=="{R}"){
              if(line>>word)
                std::cout<<"Error: unexpected identifier "<<word<<" after operand, at line "<<c+1<<"\n"<<str;
            }
        }
        else if(MOT.find(temp+" "+word)!=MOT.end()){
          control = MOT[temp+" "+word[0]];
                cout <<toHex(lc, 2)       <<"| "<<control.opcode;
          objhexfile <<control.opcode <<" ";
          lc += control.size;

          if(control.format=="{R,C}"){
            string operand = word.substr(str.find(',')+1, str.size());
            var_lc = search_symbol(operand);
            if(var_lc == -1){
                  if(is_number(operand)){
                    objhexfile <<toHex(atoi(operand.c_str()), 2)<<" ";
                          cout <<toHex(atoi(operand.c_str()), 2)<<" ";
                  }else{
                    cout<<"Error, symbol "<<operand<<" not found in symbol table "<<endl;
                  }
            }
          else{
              objhexfile <<" "<<toHex(var_lc, 2);
              objbinfile <<" "<<toBin(toHex(var_lc, 2));
          }
          if(line>>word)
              std::cout<<"Error: unexpected identifier "<<word<<" after operand, at line "<<c+1<<"\n"<<str;
          }
          else if(control.format=="{R}"){
            if(line>>word)
              std::cout<<"Error: unexpected identifier "<<word<<" after opcode, at line "<<c+1<<"\n"<<str;
          }

       }else{
         infile >> word;
         int bytes = 0;
         if(!(POT.find(word)==POT.end()))
           bytes = POT[word].size;
         else
             cout<<"Error psuedocode symbol not found"<<endl;
 				infile >> word;
 				objhexfile <<toHex(lc,2)       <<"| "<<data_break(word, bytes)       <<endl;
         objbinfile <<toBin(toHex(lc,2))<<"| "<<toBin(data_break(word, bytes))<<endl;
 				size = size_evaluation(word, bytes);
 				lc += size;
       }
    }
   }

    cout<< endl;
  }
}


std::string toString(int i){
    std::stringstream ss;
    ss << i;
    return ss.str();
}


std::string toHex(int i){
    std::stringstream ss;
    ss << std::hex << i;
    std::string s = ss.str();
    if(s.size()==1) s="0"+s;
    return s;
}

std::string toHex(int i, int bytes){
  if(bytes<0){
    std::cout<<"invalid size, not comparable"<<std::endl;
    return "\0";
  }
  if(bytes==0) bytes=1;
    std::stringstream ss;
    ss << std::hex << i;
    std::string s = ss.str();
    while(s.length()!=2*bytes)
      s="0"+s;
    return s;
}

std::string toBin(std::string hex){
  std::string binary="";
  for(int i=0; i<hex.size(); i++){
        switch (hex[i]) {
          case '0': binary+="0000"; break; case '1': binary+="0001"; break; case '2': binary+="0010"; break; case '3': binary+="0011"; break;
          case '4': binary+="0100"; break; case '5': binary+="0101"; break; case '6': binary+="0110"; break; case '7': binary+="0111"; break;
          case '8': binary+="1000"; break; case '9': binary+="1001"; break; case 'A': binary+="1010"; break; case 'a': binary+="1010"; break;
          case 'B': binary+="1011"; break; case 'b': binary+="1011"; break; case 'C': binary+="1100"; break; case 'c': binary+="1100"; break;
          case 'D': binary+="1101"; break; case 'd': binary+="1101"; break; case 'E': binary+="1110"; break; case 'e': binary+="1110"; break;
          case 'F': binary+="1111"; break; case 'f': binary+="1111"; break;  default: binary+=hex[i]; break;
        }
  }
  return binary;
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
    while(str.find(from) != std::string::npos)
     str.replace(str.find(from), from.length(), to);
    return true;
}
