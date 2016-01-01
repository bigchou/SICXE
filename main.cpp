#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include <map>
using namespace std;
struct Base{
    string name = "unnamed";
    int address = 0;
};
struct Len{
    int start = 0;
    int locctr = 0;
};
struct Src_stmt{//Source_Statement
    string label = "unnamed", opcode = "unnamed", operand = "unnamed";
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
bool Pass1(Len &len , map<string, OPTAB> &optab, map<string, SYMTAB> &symtab){
    string filename = "testdata.txt";
    //cout << "Input the filename...> ";
    //getline(cin,filename);
    ifstream ifile(filename);
    if(!ifile){
        cout << filename << " is not found!" << endl;
        return false;
    }
    // BEGIN
    string str;
    Src_stmt stmt;
    Base base;
    getline(ifile,str); // Read First Input Line
    stmt = parseSrc_Stmt(str);
    if(stmt.opcode == "START"){
        len.start = parseInt(stmt.operand);// save #[OPERAND] as starting address
        len.locctr = len.start;//Initial LOCCTR to starting address

        cout << len.locctr << ' ' << stmt.label << ' ' << stmt.opcode << ' ' << stmt.operand << endl;

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


            // ===================================================
            if(stmt.opcode == "BASE"){
                base.name = stmt.operand;
                cout << '\t' << stmt.label << ' ' << stmt.opcode << ' ' << stmt.operand << endl;
                getline(ifile,str);// read next input line
                stmt = parseSrc_Stmt(str);
                continue;
            }

            cout << len.locctr << ' ' << stmt.label << ' ' << stmt.opcode << ' ' << stmt.operand << endl;
            // =============================================
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
            }else if(stmt.opcode == "BYTE"){
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

    cout << len.locctr << ' ' << stmt.label << ' ' << stmt.opcode << ' ' << stmt.operand << endl;
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
//    for(MyOPTAB::const_iterator iter = optab.begin();iter!=optab.end();++iter)
//        cout << iter->first << '\t' << iter->second.mnemonic << ' ' << iter->second.type << ' ' << iter->second.opcode << endl;
    return true;
}
int main(){
    MyOPTAB optab;
    if(!BuildOPTAB(optab)) return 0;
    //cout << opttab["LDCH"].mnemonic << ' ' << opttab["LDCH"].type << ' ' << opttab["LDCH"].opcode << endl;
    Len len;
    MySYMTAB symtab;
    if(!Pass1(len,optab,symtab)) return 0;

    system("pause");
    return 0;
}
