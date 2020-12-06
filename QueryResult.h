#ifndef QUERY_RESULT_H
#define QUERY_RESULT_H
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <set>
using std::ostream;
using std::string;
using std::vector;
using std::shared_ptr;
using std::set;
using std::endl;
using line_no = vector<string>::size_type;
using iter = std::set<line_no>::iterator;
class QueryResult {
    friend ostream &print(ostream &os, const QueryResult &result);
public:
    QueryResult(
        string s,
        shared_ptr<set<line_no>> p,
        shared_ptr<vector<string>> f):
        sought(s), lines(p), file(f) { }
    iter begin() const {
        return lines->begin();
    }
    iter end() const {
        return lines->end();
    }
    shared_ptr<vector<string>> get_file() const {
        return file;
    }
private:
    string sought;
    shared_ptr<set<line_no>> lines;
    shared_ptr<vector<string>> file;
};

ostream &print(ostream &os, const QueryResult &result) {
    os << result.sought << " occurs " << result.lines->size()
    << " times " << endl;
    for (auto num : *result.lines) {
        os << "\t(line " << num + 1 << ") "
        << *(result.file->begin() + num) << endl;
    }
    return os;
}
#endif