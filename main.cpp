#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <map>
#define next_line_size 8
using namespace std;
struct Base{
    string name = "unnamed";
    int address = 0;
};
struct Len{
    int start = 0, locctr = 0, total = 0;
};
struct Src_stmt{//Source_Statement
    string label = "unnamed", opcode = "unnamed", operand = "unnamed";
    int address = 0;// used for pass2
};
struct OPTAB{
    string mnemonic;
    int type = 0;
    string opcode;
};
struct SYMTAB{
    string name = "unnamed";
    int address = 0;
};
struct TRecord{
    string machine_code = "000000";
};
struct TRecords{
    int start = 0;
    int num_trecord = 0;
    TRecord trecord[next_line_size];
};
typedef map<string, SYMTAB> MySYMTAB;
typedef map<string, OPTAB> MyOPTAB;

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
        stmt.opcode = tok[0];
        stmt.operand = tok[1];
    }else if(count == 3){
        stmt.label = tok[0];
        stmt.opcode = tok[1];
        stmt.operand = tok[2];
    }else{//should be like as RSUB
        stmt.label = " ";
        stmt.opcode = tok[0];
        stmt.operand = " ";
    }
    return stmt;
}
bool Pass1(Len &len , map<string, OPTAB> &optab, map<string, SYMTAB> &symtab,Base &base){
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
    if(stmt.opcode == "START"){
        len.start = parseInt(stmt.operand);// save #[OPERAND] as starting address
        len.locctr = len.start;//Initial LOCCTR to starting address
        cout << std::hex << len.locctr << '\t' << stmt.label << '\t' << stmt.opcode << '\t' << stmt.operand << endl;
        ofile << len.locctr << '\t' << stmt.label << '\t' << stmt.opcode << '\t' << stmt.operand << endl;//write line to intermediate file
        getline(ifile,str); // Read Next Input Line
        stmt = parseSrc_Stmt(str);
    }else{
        len.locctr = 0; // initialize LOCCTR to 0
    }
    // ========================================================================================================
    while(stmt.opcode != "END"){// while OPCODE != 'END' do
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
                    //cout << symtab[temp.name].name << ' ' <<  symtab[temp.name].address << endl;
                }
            }


            // ================= SetBase =============================
            if(stmt.opcode == "BASE"){
                base.name = stmt.operand;
                cout << '\t' << stmt.label << '\t' << stmt.opcode << '\t' << stmt.operand << endl;
                getline(ifile,str);// read next input line
                stmt = parseSrc_Stmt(str);
                continue;
            }else if(base.name == stmt.label){
                base.address = len.locctr;
            }
            // =============================================
            cout << std::hex << len.locctr << '\t' << stmt.label << '\t' << stmt.opcode << '\t' << stmt.operand << endl;//write line to intermediate file
            ofile << len.locctr << '\t' << stmt.label << '\t' << stmt.opcode << '\t' << stmt.operand << endl;

            bool found = false;
            string mnemonic = stmt.opcode;
            if(mnemonic[0] == '+') mnemonic = mnemonic.substr(1,stmt.opcode.size());
            //search OPTAB for OPCODE
            found = optab.count(mnemonic);
            if(found){// if found then...
                int val = optab[mnemonic].type;
                if(val >= 3)
                    len.locctr += ((stmt.opcode[0] == '+') ? val+1 : val);// add {instruction length} to LOCCTR
                else
                    len.locctr += val;
            }else if(stmt.opcode == "WORD"){
                len.locctr+=3; // add 3 to LOCCTR
            }else if(stmt.opcode == "RESW"){
                len.locctr += (3*parseInt(stmt.operand));
            }else if(stmt.opcode == "RESB"){
                len.locctr += parseInt(stmt.operand);
            }else if(stmt.opcode == "BYTE"){// find length of constant in bytes and add length to LOCCTR
                if(stmt.operand[0] == 'C')
                    len.locctr += (stmt.operand.size()-3);
                else if(stmt.operand[0] == 'X')
                    len.locctr += ((stmt.operand.size()-3)/2);
            }else{
                cout << "invalid operation code!" << endl;//Set error flag (invalid operation code)
                return false;
            }
        }// end {if not a comment}

        getline(ifile,str);// read next input line
        stmt = parseSrc_Stmt(str);
    }//end {while not END}
    SYMTAB temp;
    temp.address = len.locctr;
    temp.name = stmt.opcode;
    symtab.insert(MySYMTAB::value_type(temp.name,temp));
    //write last line to intermediate file
    ofile << len.locctr << '\t' << stmt.label << '\t' << stmt.opcode << '\t' << stmt.operand;
    len.total = len.locctr - len.start;//save (LOCCTR - starting address) as program length
    cout << std::hex << len.locctr << '\t' << stmt.label << '\t' << stmt.opcode << '\t' << stmt.operand << endl << "Program length = " << len.total << endl;
    cout << "PASS1 finish successfully!" << endl;
    return true;
}

string parseOPND(string opnd){
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
        if(flag && temp[count+1] == 'X'){
            temp = temp.substr(0,count);
        }
    }
    return temp;
}

bool Pass2(Len &len , map<string, OPTAB> &optab, map<string, SYMTAB> &symtab, Base &base){
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
    int address = 0;
    Src_stmt stmt;
    TRecords trecords;
    //read first input line {from intermediate file}
    ifile >> address;
    getline(ifile,str);
    stmt = parseSrc_Stmt(str);
    stmt.address = address;
    //write Header record to object program
    ofile << std::hex << "H";
    ofile << stmt.label << ' ';
    ofile.width(6);     ofile.fill('0');
    ofile << len.start;
    ofile.width(6);     ofile.fill('0');
    ofile << len.total << endl;
    //read next input line
    ifile >> address;
    getline(ifile,str);
    stmt = parseSrc_Stmt(str);
    stmt.address = address;
    //while OPCODE != "END" do
    while(stmt.opcode != "END"){
        TRecord trecord;
        string opcode;
        if(stmt.opcode[0] == '+'){
            opcode = stmt.opcode.substr(1,stmt.opcode.size());
        }else{
            opcode = stmt.opcode;
        }
        bool found = optab.count(opcode);// search OPTAB for OPCODE
        if(found){// if found then
            string opnd = parseOPND(stmt.operand);
            if(opnd != " "){//if there is a symbol in OPERAND field then
                bool found = symtab.count(opnd);// search SYMTAB for OPERAND
                if(found){//store symbol value as operand address
                    //cout << stmt.address << ' ' << stmt.opcode << endl;
                    if(stmt.opcode[0] == '+'){//format 4
                        trecord.machine_code = "00000000";
                        int val = symtab[stmt.operand].address;
                        for(int i=7;i>=3;--i){
                            int remainder = val % 16;
                            if(remainder >= 10){
                                trecord.machine_code[i] = ('A' + remainder - 10);
                            }else{
                                trecord.machine_code[i] = ('0'+remainder);
                            }
                            val /= 16;
                        }
                        //cout << stmt.opcode << ' ' << stmt.operand << ' ' << trecord.machine_code << endl;
                    }else{// format 3
                        //cout << stmt.address << ' ' << stmt.opcode << ' ' << stmt.operand << ' ';
                        int val = symtab[opnd].address;
                        //cout << val  << ' ' << optab[stmt.opcode].type << ' ';
                        val -= (stmt.address+optab[stmt.opcode].type);
                            //cout << val << endl;
                        if(val <= 2047 && val >= -2048){//PC-Relative


                        }else{//Base-Relative

                        }
                    }
                }else{//set error flag (undefined symbol)
                    cout << stmt.label << ' ' << stmt.opcode << ' ' << stmt.operand << endl;
                    //cout << "undefined symbol" << endl;
                    //return false;
                }
            }else{//store 0 as operand address (like as RSUB)
                //cout << stmt.opcode << endl;
                trecord.machine_code = "000000";
            }
            // assemble the object code instruction

        }else if(stmt.opcode == "BYTE" || stmt.opcode == "WORD"){//else if OPCODE = "BYTE" or "WORD" then
            //cout << stmt.label << ' ' << stmt.opcode << ' ' << stmt.operand << endl;
        }
        //==========================================================================================================
        if(trecords.num_trecord == next_line_size){//object code will not fit into the current text record
            //write text record to object program
            for(int i=0;i<trecords.num_trecord;++i)
                ofile << trecords.trecord[i].machine_code << endl;
            ofile << endl;
            trecords.num_trecord = 0;//initialize new text record
        }else{// add object code to text records

            ++trecords.num_trecord;
        }



        //ofile << stmt.address << ' ' << stmt.label << ' ' << stmt.opcode << ' ' << stmt.operand << endl;
        // read next input line
        ifile >> address;
        getline(ifile,str);
        stmt = parseSrc_Stmt(str);
        stmt.address = address;
     }

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
        ifile >> temp.type >> temp.opcode;
        optab.insert(MyOPTAB::value_type(temp.mnemonic,temp));
    }
    return true;
}
int main(){
    // Build table of opcode
    MyOPTAB optab;
    if(!BuildOPTAB(optab)) return 0;
    // Build table of symbol and finish pass1

    Len len;
    MySYMTAB symtab;
    Base base;
    if(!Pass1(len,optab,symtab,base)) return 0;
    system("pause");
    system("cls");

    if(!Pass2(len,optab,symtab,base)) return 0;
    // Menu
//    bool flag = true;
//    do{
//        int opt;
//        cout << std::dec << "(1) Show table of opcode" << endl
//            << "(2) Show table of symbol" << endl
//            << "(3) Show information of Base" << endl
//            << "(4) Generate the object file" << endl
//            << "Please input your option (option not mentioned above will exit)...> ";
//        cin >> opt;
//        switch(opt){
//            case 1:
//                for(MyOPTAB::const_iterator iter = optab.begin();iter!=optab.end();++iter)
//                    cout << iter->first << '\t' << ' ' << iter->second.type << ' ' << iter->second.opcode << endl;
//                break;
//            case 2:
//                for(MySYMTAB::const_iterator iter = symtab.begin();iter!=symtab.end();++iter)
//                    cout << std::hex << iter->first << '\t' << iter->second.address << endl ;
//                break;
//            case 3:
//                cout << std::hex << base.name << ' ' << base.address << endl;
//                break;
//            case 4:
//                if(!Pass2(len,optab,symtab,base)) return 0;
//                break;
//            default:
//                flag = false;
//        }
//        system("pause");
//        system("cls");
//    }while(flag);
    // Exit
    cout << "Thanks for using my assembler!" << endl;
    return 0;
}
