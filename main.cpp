#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <vector>
#include <bitset>
#include <cstring>
#define next_line_size 9
using namespace std;

struct Base{
    string name = "unnamed";
    int address = 0;
};
struct Len{
    int start = 0, locctr = 0, total = 0;
};
struct Src_stmt{//Source_Statement
    string label = "unnamed", mnemonic = "unnamed", operand = "unnamed";
    int address = 0;// used for pass2
};
struct OPTAB{
    string mnemonic;
    int type = 0;
    int opcode;
};
struct REGTAB{
    string name = "unnamed";
    int no = 0;
};
struct SYMTAB{
    string name = "unnamed";
    int address = 0;
};

struct TRecords{
    int start = 0;
    int num_trecord = 0;
    string trecord[next_line_size+1];
};
struct MRecord{
    int address = 0;
    int len = 5;
};
typedef map<string, SYMTAB> MySYMTAB;
typedef map<string, OPTAB> MyOPTAB;
typedef map<string, REGTAB> MyREGTAB;

int parseInt(const string& str){
    string tmp = str;
    reverse(tmp.begin(),tmp.end());
    int val = 0, mplier = 1;
    for(int i=0;i<tmp.size();++i){
        if(toupper(tmp[i]) >= 'A')
            val += (mplier * (toupper(tmp[i])-'A'+10));
        else
            val += (mplier * (tmp[i]-'0'));
        mplier *= 10;
    }
    return val;
}

Src_stmt parseSrc_Stmt(const string &str){
    // Determine comment
    Src_stmt stmt;
    for(int i=0;i<str.size();++i){
        if(str[i] == ' '){
            continue;
        }else if(str[i] == '.'){
            stmt.label = ".";
            return stmt;
        }else{
            break;
        }
    }
    // Determine number of tokens
    istringstream iss(str);
    string tok[3];
    int count = 0;
    do{
        iss >> tok[count++];
    }while(iss);
    --count;
    // Test
    //for(int i=0;i<count;++i) cout << tok[i] << ' ' << endl;

    if(count == 2){
        stmt.label = " ";
        stmt.mnemonic = tok[0];
        stmt.operand = tok[1];
    }else if(count == 3){
        stmt.label = tok[0];
        stmt.mnemonic = tok[1];
        stmt.operand = tok[2];
    }else{//should be like as RSUB
        stmt.label = " ";
        stmt.mnemonic = tok[0];
        stmt.operand = " ";
    }
    return stmt;
}
bool Pass1(Len &len , map<string, OPTAB> &optab, map<string, SYMTAB> &symtab){
    string filename = "testdata.txt";
    cout << "Input the filename (default: testdata.txt)...> ";
    getline(cin,filename);
    if(filename.empty()) filename = "testdata.txt";

    ifstream ifile(filename);
    if(!ifile){
        cout << filename << " is not found!" << endl;
        return false;
    }
    ofstream ofile("intermediate.txt");
    if(!ofile){
        cout << "failure to output file!" << endl;
        return false;
    }
    // BEGIN
    string str;
    Src_stmt stmt;
    getline(ifile,str); // Read First Input Line
    stmt = parseSrc_Stmt(str);
    if(stmt.mnemonic == "START"){
        len.start = parseInt(stmt.operand);// save #[OPERAND] as starting address
        len.locctr = len.start;//Initial LOCCTR to starting address
        cout << std::hex << len.locctr << '\t' << stmt.label << '\t' << stmt.mnemonic << '\t' << stmt.operand << endl;
        ofile << len.locctr << '\t' << stmt.label << '\t' << stmt.mnemonic << '\t' << stmt.operand << endl;//write line to intermediate file
        getline(ifile,str); // Read Next Input Line
        stmt = parseSrc_Stmt(str);
    }else{
        len.locctr = 0; // initialize LOCCTR to 0
    }
    // ========================================================================================================
    while(stmt.mnemonic != "END"){// while MNEMONIC != 'END' do
        if(stmt.label != "."){//if this is not a comment line then
            if(stmt.label != " "){//if there is a symbol in the LABEL field then
                bool found = symtab.count(stmt.label);// search SYMTAB for LABEL
                if(found){// if found then
                    cout << "Duplicate symbols!" << endl;//Set error flag (duplicate symbol)
                    return false;
                }else{
                    SYMTAB temp;
                    temp.address = len.locctr;
                    temp.name = stmt.label;
                    symtab.insert(MySYMTAB::value_type(temp.name,temp));// Insert (LABEL,LOCCTR) into SYMTAB
                }
            }

            cout << std::hex << len.locctr << '\t' << stmt.label << '\t' << stmt.mnemonic << '\t' << stmt.operand << endl;//write line to intermediate file
            ofile << len.locctr << '\t' << stmt.label << '\t' << stmt.mnemonic << '\t' << stmt.operand << endl;

            bool found = false;
            string mnemonic = stmt.mnemonic;
            if(mnemonic[0] == '+') mnemonic = mnemonic.substr(1,stmt.mnemonic.size());//remove '+' from format 4
            //search OPTAB
            found = optab.count(mnemonic);
            if(found){// if found then...
                int val = optab[mnemonic].type;
                if(val >= 3)
                    len.locctr += ((stmt.mnemonic[0] == '+') ? val+1 : val);// add {instruction length} to LOCCTR
                else
                    len.locctr += val;
            }else if(stmt.mnemonic == "BASE"){
                getline(ifile,str);// read next input line
                stmt = parseSrc_Stmt(str);
                continue;
            }else if(stmt.mnemonic == "WORD"){
                len.locctr+=3; // add 3 to LOCCTR
            }else if(stmt.mnemonic == "RESW"){
                len.locctr += (3*parseInt(stmt.operand));
            }else if(stmt.mnemonic == "RESB"){
                len.locctr += parseInt(stmt.operand);
            }else if(stmt.mnemonic == "BYTE"){// find length of constant in bytes and add length to LOCCTR
                if(stmt.operand[0] == 'C')
                    len.locctr += (stmt.operand.size()-3);
                else if(stmt.operand[0] == 'X')
                    len.locctr += ((stmt.operand.size()-3)/2);
            }else{
                cout << "invalid operation code!" << endl;//Set error flag (invalid operation code)
                return false;
            }
        }// end {if not a comment}
        else{
            cout << endl;
        }
        getline(ifile,str);// read next input line
        stmt = parseSrc_Stmt(str);
    }//end {while not END}
    SYMTAB temp;
    temp.address = len.locctr;
    temp.name = stmt.mnemonic;
    symtab.insert(MySYMTAB::value_type(temp.name,temp));
    //write last line to intermediate file
    ofile << len.locctr << '\t' << stmt.label << '\t' << stmt.mnemonic << '\t' << stmt.operand;
    len.total = len.locctr - len.start;//save (LOCCTR - starting address) as program length
    cout << std::hex << len.locctr << '\t' << stmt.label << '\t' << stmt.mnemonic << '\t' << stmt.operand << endl << "Program length = " << len.total << endl;
    cout << "PASS1 finish successfully!" << endl;
    return true;
}

string parseOPND(string &opnd){
    string temp = opnd;
    if(opnd[0] == '#' || opnd[0] == '@'){// Determine prefix
        temp = temp.substr(1,temp.size());
    }else{// Determine index_addressing
        int count = 0;
        bool flag = false;
        for(int i=0;i<opnd.size();++i){
            if(temp[i] == ','){
                count = i;
                flag = true;
                break;
            }
        }
        if(flag){//flag && temp[count+1] == 'X'
            temp = temp.substr(0,count);
        }
    }
    //cout << temp << endl;
    return temp;
}
enum AdrMode{immediate, undirect, index, simple};
AdrMode GetAdrMode(string &opnd){
    if(opnd[0] == '#')
        return immediate;
    else if(opnd[0] == '@'){
        return undirect;
    }else{
        for(int i=0;i<opnd.size();++i){
            if(opnd[i] == ',')
                return index;
        }
        return simple;
    }
}

string bin_str_to_hex_str(string bin_str){
    int count = bin_str.size() /4;
    string hex_str;
    for(int i=0;i<count;++i){
        int v = 8;
        int total = 0;
        for(int j=0;j<4;++j){
            total += v*(bin_str[j+4*i]-'0');
            v/= 2;
        }
        hex_str += ( total>9 ? 'A'+total-10 : '0'+total );
    }
    return hex_str;
}

int strDec_to_dec(string str){
    if(str[0]=='#'){
        str = str.substr(1,str.size());
    }
    reverse(str.begin(), str.end());
    int val = 0;
    int base = 1;
    for(int i=0;i<str.size();++i){
        val += base * (str[i]-'0');
        base *= 10;

    }
    return val;
}


bool Pass2(Len &len , map<string, OPTAB> &optab, map<string, SYMTAB> &symtab, map<string, REGTAB> &regtab){
    string filename = "intermediate.txt";
    ifstream ifile(filename);
    if(!ifile){
        cout << filename << "is not found!" << endl;
        return false;
    }
    ofstream ofile("a.obj");
    if(!ofile){
        cout << "failure to output file!" << endl;
        cout << false;
    }
    // BEGIN
    string str;
    bool EnablePrint = false;
    Base base;
    len.locctr = 0;
    Src_stmt stmt;
    TRecords trecords;
    vector<MRecord> mrecords;
    trecords.start = len.start;
    //read first input line {from intermediate file}
    ifile >> len.locctr;
    getline(ifile,str);
    stmt = parseSrc_Stmt(str);
    stmt.address = len.locctr;
    // write listing line
    cout << '\t' << std::hex << stmt.label << '\t' << stmt.mnemonic << '\t' << stmt.operand << endl;
    //write Header record to object program
    ofile << std::hex << "H";
    ofile << stmt.label << ' ';
    ofile.width(6);     ofile.fill('0');
    ofile << len.start;
    ofile.width(6);     ofile.fill('0');
    ofile << len.total << endl;
    //read next input line
    ifile >> len.locctr;
    getline(ifile,str);
    stmt = parseSrc_Stmt(str);
    stmt.address = len.locctr;

    //while OPCODE != "END" do
    string asm_instr, asm_opcode, asm_nixbpe, asm_adr;
    while(stmt.mnemonic != "END"){
        asm_opcode.clear(); asm_nixbpe.clear(); asm_adr.clear();
        string mnemonic = (stmt.mnemonic[0] == '+') ? stmt.mnemonic.substr(1,stmt.mnemonic.size()) : stmt.mnemonic;//remove '+' from format 4
        bool found = optab.count(mnemonic);// search OPTAB for OPCODE
        if(found){// else could be 1. BASE directive 2. WORD or BYTE 3. even ERROR
            string opnd = parseOPND(stmt.operand);
            if(opnd != " "){//if there is a symbol in OPERAND field then... else do RSUB!
                bool found = symtab.count(opnd);// search SYMTAB for OPERAND
                if(found){// else would be REGISTER or Constant
                    if(stmt.mnemonic[0] == '+'){//format 4
                        asm_opcode = bitset<6>(optab[mnemonic].opcode).to_string();
                        asm_adr = bitset<20>(symtab[opnd].address).to_string();
                        AdrMode adrmode = GetAdrMode(stmt.operand);
                        if(adrmode == immediate){
                            asm_nixbpe = "010001";
                        }else if(adrmode == undirect){
                            asm_nixbpe = "100001";
                        }else if(adrmode == index){
                            asm_nixbpe = "001001";
                        }else if(adrmode == simple){
                            asm_nixbpe = "110001";
                        }
                        // Adjustment(MRecord)
                        MRecord temp;
                        temp.address = stmt.address+1;
                        mrecords.push_back(temp);
                    }else{// format 3
                        asm_opcode = bitset<6>(optab[mnemonic].opcode).to_string();
                        int offset = symtab[opnd].address;//Calculate Offset
                        offset -= (stmt.address+optab[stmt.mnemonic].type);
                        //cout << std::dec << offset << endl;
                        if(offset <= 2047 && offset >= -2048){//PC-Relative
                            asm_adr = bitset<12>(offset).to_string();
                            AdrMode adrmode = GetAdrMode(stmt.operand);
                            if(adrmode == immediate){
                                asm_nixbpe = "010010";
                            }else if(adrmode == undirect){
                                asm_nixbpe = "100010";
                            }else if(adrmode == index){
                                asm_nixbpe = "001010";
                            }else if(adrmode == simple){
                                asm_nixbpe = "110010";
                            }
                        }else{// Base_relative or long_address
                            offset = symtab[opnd].address;
                            offset -= base.address;
                            if(offset >= 0 && offset <= 4095){//Base-Relative
                                offset = symtab[opnd].address;
                                offset -= base.address;
                                asm_adr = bitset<12>(offset).to_string();
                                AdrMode adrmode = GetAdrMode(stmt.operand);
                                if(adrmode == immediate){
                                    asm_nixbpe = "010100";
                                }else if(adrmode == undirect){
                                    asm_nixbpe = "100100";
                                }else if(adrmode == index){
                                    asm_nixbpe = "111100";
                                }else if(adrmode == simple){
                                    asm_nixbpe = "110100";
                                }
                            }else{// long_address_error
                                cout << "Cannot do addressing because of long address" << endl;
                                return false;
                            }
                        }
                    }
                }else{//REGISTER or Constant
                    bool found = regtab.count(opnd);
                    if(found){//Register(format 2)
                        bool flag = false;
                        for(int i=0;i<stmt.operand.size();++i){// determine one or two registers
                            if(stmt.operand[i] == ','){
                                flag = true;
                                break;
                            }
                        }
                        if(flag){// two register
                            asm_opcode = bitset<6>(optab[mnemonic].opcode).to_string();
                            asm_adr = bitset<6>(regtab[opnd].no).to_string();
                            int i = 0;
                            for(i;i<stmt.operand.size();++i)
                                if(stmt.operand[i] == ','){
                                    ++i;
                                    break;
                                }
                            for(i;i<stmt.operand.size();++i)
                                if(stmt.operand[i] != ' ')break;
                            string reg = stmt.operand.substr(i,stmt.operand.size());
                            asm_adr += bitset<4>(regtab[reg].no).to_string();
                        }else{// one Register
                            asm_opcode = bitset<6>(optab[mnemonic].opcode).to_string();
                            asm_adr = bitset<6>(regtab[opnd].no).to_string();
                            asm_adr += "0000";
                        }

                    }else{//Constant
                        if(stmt.mnemonic[0] == '+'){//Constant in format 4 with immediate addressing mode
                            asm_opcode = bitset<6>(optab[mnemonic].opcode).to_string();
                            asm_nixbpe = "010001";
                            asm_adr = bitset<20>(strDec_to_dec(opnd)).to_string();
                        }else{
                            asm_opcode = bitset<6>(optab[mnemonic].opcode).to_string();
                            asm_nixbpe = "010000";
                            asm_adr = bitset<12>(strDec_to_dec(stmt.operand)).to_string();
                        }
                    }
                }
            }else{//store 0 as operand address (like as RSUB)
                asm_opcode = bitset<6>(optab[mnemonic].opcode).to_string();
                asm_nixbpe = "110000";
                for(int i=0;i<12;++i) asm_adr += '0';
            }
            // assemble the object code instruction
            asm_instr = asm_opcode + asm_nixbpe + asm_adr;
            asm_instr = bin_str_to_hex_str(asm_instr);

        }else if(stmt.mnemonic == "BYTE" || stmt.mnemonic == "WORD"){//else if OPCODE = "BYTE" or "WORD" then
            asm_instr.clear();
            if(stmt.operand[0] == 'C'){
                for(int i=2; i < stmt.operand.size()-1;++i){
                    stringstream stream;
                    stream << std::hex << (int)stmt.operand[i];
                    string result(stream.str());
                    for(int j=0;j<result.size();++j)
                        result[j] = toupper(result[j]);
                    asm_instr += result;
                }
            }else if(stmt.operand[0] == 'X'){
                for(int i=2; i < stmt.operand.size()-1;i+=2){
                    stringstream stream;
                    string temp = stmt.operand.substr(i,2);
                    stream << std::hex << temp;
                    string result(stream.str());
                    for(int j=0;j<result.size();++j)
                        result[j] = toupper(result[j]);
                    asm_instr += result;
                }
            }
        }else if(stmt.mnemonic == "BASE"){
            cout << '\t' << std::hex << stmt.label << '\t' << stmt.mnemonic << '\t' << stmt.operand << endl;
            base.address = symtab[stmt.operand].address;
            base.name = symtab[stmt.operand].name;
            // read next line
            ifile >> len.locctr;
            getline(ifile,str);
            stmt = parseSrc_Stmt(str);
            stmt.address = len.locctr;
            continue;
        }else if(stmt.mnemonic == "RESB" || stmt.mnemonic == "RESW"){
            cout << '\t' << std::hex << stmt.label << '\t' << stmt.mnemonic << '\t' << stmt.operand << endl;
            if(EnablePrint == true){// Write text record to object program
                ofile << "T";
                ofile.width(6);     ofile.fill('0');
                ofile << trecords.start;
                ofile.width(2);     ofile.fill('0');
                ofile << stmt.address - trecords.start;
                for(int i=0;i<trecords.num_trecord;++i)
                ofile << trecords.trecord[i];
                ofile << endl;
                trecords.num_trecord = 0;//initialize new text record
                trecords.start = stmt.address;
                EnablePrint = false;
            }
            // read next line
            ifile >> len.locctr;
            getline(ifile,str);
            stmt = parseSrc_Stmt(str);
            stmt.address = len.locctr;
            trecords.start = stmt.address;
            continue;
        }
        else{
            cout << "Mnemonic not found!" << endl;
            return false;
        }

        //==========================================================================================================

        if(trecords.num_trecord == next_line_size){//object code will not fit into the current text record
            //write text record to object program
            ofile << "T";
            ofile.width(6);     ofile.fill('0');
            ofile << trecords.start;
            ofile.width(2);     ofile.fill('0');
            ofile << stmt.address - trecords.start;
            for(int i=0;i<trecords.num_trecord;++i)
                ofile << trecords.trecord[i] ;
            ofile << endl;
            trecords.num_trecord = 0;//initialize new text record
            trecords.start = stmt.address;
            trecords.trecord[trecords.num_trecord++] = asm_instr;
            EnablePrint = true;
        }else{// add object code to text records
            trecords.trecord[trecords.num_trecord++] = asm_instr;
            EnablePrint = true;
        }


        // write listing line
        cout << stmt.address << '\t' << stmt.label << '\t' << stmt.mnemonic << '\t' << stmt.operand << '\t' << asm_instr << endl;
        // read next input line
        ifile >> len.locctr;
        getline(ifile,str);
        stmt = parseSrc_Stmt(str);
        stmt.address = len.locctr;
    }
    // Write last Text record to object program
    ofile << "T";
    ofile.width(6);     ofile.fill('0');
    ofile << trecords.start;
    ofile.width(2);     ofile.fill('0');
    ofile << stmt.address - trecords.start;
    for(int i=0;i<trecords.num_trecord;++i)
        ofile << trecords.trecord[i] ;
    ofile << endl;
    // Adjustment (MRecord)
    for(int i=0;i<mrecords.size();++i){
        ofile << "M";
        ofile.width(6);     ofile.fill('0');
        ofile << mrecords[i].address;
        ofile.width(2);     ofile.fill('0');
        ofile << mrecords[i].len << endl;
    }
    // Write End record to object program
    ofile << "E";
    ofile.width(6);     ofile.fill('0');
    ofile <<len.start;
    // Write last listing line
    cout << stmt.address << '\t' << stmt.label << '\t' << stmt.mnemonic << '\t' << stmt.operand << '\t' << endl;
    return true;
}

bool BuildOPTAB(map<string, OPTAB> &optab){
    ifstream ifile("OPTAB.txt");
    if(!ifile){
        cout << "Opcode Table is not found. Please find it and execute again!" << endl;
        return false;
    }
    OPTAB temp;
    while(ifile >> temp.mnemonic){
        ifile >> temp.type >> std::hex >> temp.opcode;
        temp.opcode /= 4;
        optab.insert(MyOPTAB::value_type(temp.mnemonic,temp));
    }
    return true;
}
bool BuildREGTAB(map<string, REGTAB> &regtab){
    ifstream ifile("REGTAB.txt");
    if(!ifile){
        cout << "Register Table is not found. Please find it and execute again!" << endl;
        return false;
    }
    REGTAB temp;
    while(ifile >> temp.name){
        ifile >> std::dec >> temp.no;
        regtab.insert(MyREGTAB::value_type(temp.name,temp));
    }
    return true;
}
int main(){
    // Build table of opcode
    MyOPTAB optab;
    MyREGTAB regtab;
    if(!BuildOPTAB(optab)) return 0;
    if(!BuildREGTAB(regtab)) return 0;
    Len len;
    MySYMTAB symtab;
    if(!Pass1(len,optab,symtab)) return 0;
    system("pause");
    system("cls");
    // Menu
    bool flag = true;
    do{
        int opt;
        cout << std::dec << "(1) Show table of opcode" << endl
            << "(2) Show table of symbol" << endl
            << "(3) Show table of register" << endl
            << "(4) Generate the object file and exit the program" << endl
            << "Please input your option (option not mentioned above will exit)...> ";
        cin >> opt;
        switch(opt){
            case 1:
                for(MyOPTAB::const_iterator iter = optab.begin();iter!=optab.end();++iter)
                    cout << iter->first << '\t' << ' ' << iter->second.type << ' ' << std::hex << iter->second.opcode*4 << endl;
                break;
            case 2:
                for(MySYMTAB::const_iterator iter = symtab.begin();iter!=symtab.end();++iter)
                    cout << std::hex << iter->first << '\t' << iter->second.address << endl ;
                break;
            case 3:
                for(MyREGTAB::const_iterator iter = regtab.begin();iter!=regtab.end();++iter)
                    cout << iter->first << '\t' << ' ' << iter->second.no << endl;
                break;
            case 4:
                flag = false;
                if(!Pass2(len,optab,symtab,regtab))return 0;
                break;
            default:
                flag = false;
                if(!Pass2(len,optab,symtab,regtab))return 0;
        }
        system("pause");
        system("cls");
    }while(flag);
    // Exit
    cout << "Thanks for using my assembler!" << endl;
    system("pause");
    return 0;
}
