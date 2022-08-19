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
