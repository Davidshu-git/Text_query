#include <fstream>
#include <iostream>
#include <string>
#include "TextQuery.h"
#include "QueryResult.h"
using std::ifstream;
using std::cout;
using std::cin;
using std::string;
using std::ostream;
using std::endl;

void run_queries(ifstream &infile){
    TextQuery tq(infile);
    while (true)
    {
        cout << "enter word to look for, or q to quit: ";
        string s;
        if (!(cin >> s) || s == "q")
        {
            break;
        }
        Query q = Query("apple") & Query("pipi") | Query("and");
        print(cout, q.eval(tq)) << endl;
    }
}

int main(int argc, char *argv[]) {
    ifstream infile;
    if (argc == 2)
    {
        infile.open(argv[1]);
        run_queries(infile);
        infile.close();
    } else {
        cout << "open file error" << endl;
    }
}