#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <iomanip>

using namespace std;

string toString(int i){
    stringstream ss;
    ss << i;
    return ss.str();
}

string toHex(int i){
    stringstream ss;
    ss << hex << i;
    string s = ss.str();
    if(s.size()==1) s="0"+s;
    return s;
}

string toHex(int i, int bytes){
  if(bytes==0) bytes=1;
  //cout<<"got i , bytes : "<<i<<" "<<bytes<<endl;
    stringstream ss;
    ss << hex << i;
    string s = ss.str();
    while(s.length()!=2*bytes)
      s="0"+s;
    return s;
}

string toBin(string hex){
  string binary="";

  for(int i=0; i<hex.size(); i++){
        switch (hex[i]) {
          case '0': binary+="0000"; break;
          case '1': binary+="0001"; break;
          case '2': binary+="0010"; break;
          case '3': binary+="0011"; break;
          case '4': binary+="0100"; break;
          case '5': binary+="0101"; break;
          case '6': binary+="0110"; break;
          case '7': binary+="0111"; break;
          case '8': binary+="1000"; break;
          case '9': binary+="1001"; break;
          case 'A': binary+="1010"; break; case 'a': binary+="1010"; break;
          case 'B': binary+="1011"; break; case 'b': binary+="1011"; break;
          case 'C': binary+="1100"; break; case 'c': binary+="1100"; break;
          case 'D': binary+="1101"; break; case 'd': binary+="1101"; break;
          case 'E': binary+="1110"; break; case 'e': binary+="1110"; break;
          case 'F': binary+="1111"; break; case 'f': binary+="1111"; break;
          default:
           binary+=hex[i]; break;
        }
  }
  return binary;
}

struct mnemonic{//MOT Table format
  string name;
  int    size;
  string opcode;
  string format;
}mot[13];

struct psuedocode{//POT Table format
	string name;
	int    size;
}pot[9];

struct symbol{ //Symbol Table format
	string name;
	string type;
	int location;
	int size;
	int section_id;
	string is_global;
};

struct section{ //Section Table format
	int id;
	string name;
	int size;
};





vector<symbol> symlab; //Symbol Table
vector<section> sec; //Section Table

int lc = 0; //Controls Location Counter
int sec_id = 0; //Manage section Id
int var_lc; //Store location of variable in Pass2
ifstream infile; //Input File Stream
ofstream inpfile; //Output file stream
ofstream outfile; //Output File Stream
ofstream objhexfile;
ofstream objbinfile;
string word; //Read Word by Word from file
string temp; //Temporary Variable
mnemonic control;
int size;  //Control Variable for search
unordered_map<string, mnemonic  > MOT;
unordered_map<string, psuedocode> POT;


void init()
{
	//Initializing Machine Opcode Table
  //       Mnemonic Size Opcode Format
  mot[ 0] = {"ADD",  1, "01", "{R}"  };
	mot[ 1] = {"ADDI", 5, "02", "{C}"  };
	mot[ 2] = {"CMP",  5, "03", "{R,C}"};
	mot[ 3] = {"INC",  1, "04", "{R}"  };
	mot[ 4] = {"JE",   5, "05", "{C}"  };
	mot[ 5] = {"JMP",  5, "06", "{C}"  };
	mot[ 6] = {"LOAD", 5, "07", "{C}"  };
	mot[ 7] = {"LOADI",1, "08", "{}"   };
	mot[ 8] = {"MVI",  5, "09", "{R,C}"};
	mot[ 9] = {"MOV",  1, "0A", "{R,R}"};
	mot[10] = {"STOP", 1, "0B", "{}"   };
	mot[11] = {"STORE",5, "0C", "{C}"  };
	mot[12] = {"STORI",1, "0D", "{}"  };

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

  for(int i=0; i<13; i++)
    MOT[mot[i].name]=mot[i];

  for(int i=0; i<10; i++)
   POT[pot[i].name]=pot[i];

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
		if(data[i] == ',')
			size += bytes;
	}
	//size += 4;
	return size;
}

string data_break(string data, int bytes) //Convert String of Input Number into Binary String
{
	string final="";
	string temporary = "";
	for(int i = 0;i < data.length();i++)
	{
		if(data[i] == ',')
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
   return 0;
  else if(word[0]=='B')
   return 1;
  else if(word[0] == 'C')
   return 2;
  else if(word[0] == 'I')
   return 3;
  return -1;
}

void store_symlab() //Storing Symbol Table in File
{
  cout<<"\n\nSYMBOL TABLE\n-----------------------"<<endl;
	outfile.open("symbol.csv");
	outfile << "Name,Type,Location,Size,SectionID,IsGlobal\n";
  cout<<setw(8)<<"Name "<<setw(8)<<"Type "<<setw(9)<<"Location"<<setw(4)<<" Size "<<setw(10)<<"SectionID ";
  cout<<setw(8)<<"IsGlobal"<<"\n";
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
  cout<<setw(4)<<"ID"<<setw(8)<<"Name"<<setw(5)<<"Size"<<"\n";
	for(vector<section>::const_iterator i = sec.begin();i != sec.end();++i)
	{
		outfile << i->id<<",";     cout<<setw(4)<<i->id<<" ";
		outfile << i->name<<",";   cout<<setw(8)<<i->name<<" ";
		outfile << i->size<<"\n";  cout<<setw(5)<<i->size<<"\n";
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


void input(){
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
    cout<<"Enter the filename: ";
    cin>>line;
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

void pass1()
{
	infile.open("input.txt");
	while(infile >> word)
	{
    //cout<<"processing word "<<word<<endl; //debug
		if(MOT.find(word)==MOT.end())
		{
			temp = word;
			if(word.find(":") != -1)//Label is Found
			{
				symlab.push_back({temp.erase(word.length()-1,1),"label",lc,-1,sec_id,"false"}); //Inserting into Symbol Table
			}
			else if(word == "section")//Section is Found
			{
				infile >> word;
				sec_id++;
				sec.push_back({sec_id,word,0}); //Inserting into Section Table
				if(sec_id != 1) // Updating previous section Size
				{
					sec[sec_id-2].size = lc;
					lc = 0;
				}
			}
			else if(word == "global") //Global Varaible is Found
			{
				infile >> word;
				symlab.push_back({word,"label",-1,-1,-1,"true"}); //Inserting into Symbol Table
			}
			else if(word == "extern") //External Variable is found
			{
				infile >> word;
				symlab.push_back({word,"external",-1,-1,-1,"false"}); //Inserting into Symbol Table
			}
			else//Variable is Found
			{
				infile >> word;
				infile >> word;
				size = size_evaluation(word);
				symlab.push_back({temp,"var",lc,size,sec_id,"false"}); //Inserting into Symbol Table
				lc += size;
			}
		}
		else
		{
      control = MOT[word];
      //LOADI and STOREI do not have any paramenter
			if(!(control.format=="{}" && control.name!="STOP"))
				infile >> word;
      //if(word=="MVI" || word=="MOV" || word == "CMP")
			if(control.format=="{R,C}" || control.format=="{R,R}")
				infile >> word;
			lc += control.size;
		}
	}

	sec[sec_id-1].size = lc; //Updating size of current Section

	store_symlab();
	store_sec();

	infile.close();
}

void pass2()
{
	infile.open("input.txt");
	objhexfile.open("outputhex.txt");
  objbinfile.open("outputbin.txt");
	while(infile >> word)
	{
    //cout<<"Processing the word "<<word<<endl;
    //char ch; cin>>ch;
		if(MOT.find(word)==MOT.end())
		{
      //cout<<word<<" not in machine opcode table"<<endl;
			temp = word;
			if(word.find(":") != -1) //No Machine Code for Label
 			{
 				objhexfile << "";
        objbinfile << "";
			}
			else if(word == "global") //No change in Global content
			{
				infile >> word;
				objhexfile <<"global "<<word<<endl;
        objbinfile <<"global "<<word<<endl;
			}
			else if(word == "extern") //No change in External Content
			{
				infile >> word;
				objhexfile <<"extern "<<word<<endl;
        objbinfile <<"extern "<<word<<endl;
			}
			else if(word == "section") //No change in Section content
			{
				infile >> word;
				objhexfile <<"section ."<<word<<endl;
        objbinfile <<"section ."<<word<<endl;
				lc = 0;
			}
			else //Variables are converted to hex along with the values
			{
				infile >> word;
        int bytes = 0;
        if(!(POT.find(word)==POT.end())){
          bytes = POT[word].size;
        }
				infile >> word;
				objhexfile <<toHex(lc)       <<" "<<data_break(word, bytes)       <<endl;
        objbinfile <<toBin(toHex(lc))<<" "<<toBin(data_break(word, bytes))<<endl;
				size = size_evaluation(word, bytes);
				lc += size;
			}
		}
		else
		{
      mnemonic control = MOT[word];
			objhexfile <<toHex(lc)       <<" "<<control.opcode;
      objbinfile <<toBin(toHex(lc))<<" "<<toBin(control.opcode);
			//if(word=="ADD"||word=="INC") //ADD and INC have defined register following it
      if(control.format=="{R}")
			{
				infile >> word;
        word=toHex(decode_register(word), 2);
				objhexfile <<" "<<word;
        objbinfile <<" "<<toBin(word);
			}
			//else if(word=="ADDI" || word=="JE" || word=="JMP" || word=="LOAD" || word=="STORE") //ADDI, JE, JMP, LOAD and STORE have one constant following it
      else if(control.format=="{C}")
			{
				infile >> word;
				var_lc = search_symbol(word);
				if(var_lc == -1){
          objhexfile <<" "<<toHex(atoi(word.c_str()), 2);
          objbinfile <<" "<<toBin(toHex(atoi(word.c_str()), 2));
        }
				else{
          objhexfile <<" "<<toHex(var_lc);
          objbinfile <<" "<<toBin(toHex(var_lc));
        }
			}
			//else if(word=="CMP" || word=="MVI") //CMP and MVI have one register and one constant following it
      else if(control.format=="{R,C}")
			{

				infile >> word;
        word=toHex(decode_register(word), 2);
        //cout<<"reached here "<<word<<endl;
				objhexfile <<" "<<word;
        objbinfile <<" "<<toBin(word);
				infile >> word;
				var_lc = search_symbol(word);
				if(var_lc == -1){
          objhexfile <<" "<<toHex(atoi(word.c_str()), 2);
          objbinfile <<" "<<toBin(toHex(atoi(word.c_str()), 2));
        }
				else{
          objhexfile <<" "<<toHex(var_lc, 2);
          objbinfile <<" "<<toBin(toHex(var_lc, 2));
        }
			}
			//else if(word == "MOV") //MOV have both registers following it
      else if(control.format=="{R,R}")
			{
				infile >> word;
        word=toHex(decode_register(word), 2);
        objhexfile <<" "<<word;
        objbinfile <<" "<<toBin(word);
				infile >> word;
        word=toHex(decode_register(word), 2);
				objhexfile <<" "<<word;
        objbinfile <<" "<<toBin(word);
			}
			lc += control.size;
			objhexfile << "\n";
      objbinfile << "\n";
		}
	}
	objhexfile.close();
  objbinfile.close();
	infile.close();
  display_file("outputhex.txt");
}


int main(){
  init();
  input();
  pass1();
  pass2();
  cout<<endl;
  return 0;
}
