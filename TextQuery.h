#ifndef TEXT_QUERY_H
#define TEXT_QUERY_H
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <algorithm>
#include "QueryResult.h"
using std::ifstream;
using std::istringstream;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;
using std::map;
using std::set;
using std::ostream;
using std::set_intersection;
using std::inserter;

// TextQuery对象会保存文件字符串数组，以及每个单词出现的行号set
class TextQuery {
public:
    using line_no = vector<string>::size_type;
    TextQuery(ifstream &infile);
    QueryResult query(const string &s) const;
private:
    shared_ptr<vector<string>> file;
    map<string, shared_ptr<set<line_no>>> wm;
};

TextQuery::TextQuery(ifstream &infile): file(new vector<string>){
    string text;
    while (getline(infile, text)) {
        file->push_back(text);
        int n = file->size() - 1;
        istringstream line(text);
        string word;
        while (line >> word)
        {
            auto &lines = wm[word];
            if (!lines)
            {
                lines.reset(new set<line_no>);
            }
            lines->insert(n);
        }
    }
}

QueryResult TextQuery::query(const string &s) const {
    static shared_ptr<set<line_no>> nodata(new set<line_no>);
    auto loc = wm.find(s);
    if (loc == wm.end()) {
        return QueryResult(s, nodata, file);
    } else {
        return QueryResult(s, loc->second, file);
    }
}

// 抽象基类
class QueryBase {
    friend class Query;
protected:
    using line_no = TextQuery::line_no;
    virtual ~QueryBase() = default;
private:
    virtual QueryResult eval(const TextQuery&) const = 0;
    virtual string rep() const = 0;
};

//WordQuery类，成员都是私有的
class WordQuery : public QueryBase {
    friend class Query;
    WordQuery(const string &s): query_word(s) { }
    //继承抽象基类，必须定义虚函数
    QueryResult eval(const TextQuery &t) const override{
        return t.query(query_word);
    }
    string rep() const override{
        return query_word;
    }
    string query_word;
};

// 隐藏基类的继承体系，调用成员函数都是经过基类指针进行调用
class Query {
    friend Query operator~(const Query &);
    friend Query operator|(const Query &, const Query &);
    friend Query operator&(const Query &, const Query &);
public:
    //用字符构造WordQuery对象，这是实现基本Query功能的第一步
    Query(const string &);
    //对Query对象的eval和rep的非虚调用在这里变成了虚调用
    //eval定义是实现query功能，返回查询的结果
    QueryResult eval(const TextQuery &text) const {
        return q->eval(text);
    }
    string rep() const {
        return q->rep();
    }
private:
    //构造函数，使用智能指针初始化
    Query(shared_ptr<QueryBase> query) : q(query) { }
    //基类指针成员
    shared_ptr<QueryBase> q;
};

//使用字符串构造WordQuery对象, 在此之前必须定义WordQuery
//使用直接初始化智能指针
//调用了WordQuery类的构造函数
inline Query::Query(const string &s): q(new WordQuery(s)) { }

//重载输出运算符，输出Query对象
ostream &operator<<(ostream &os, const Query &query) {
    //实际上由Query调用基类指针虚函数化了
    return os << query.rep();
}

//NotQuery类
class NotQuery: public QueryBase {
    friend Query operator~(const Query &);
    NotQuery(const Query &q): query(q) { }
    string rep() const override{
        return "~(" + query.rep() + ")";
    }
    QueryResult eval(const TextQuery&) const override;
    Query query;
};

//调用了Query对象的私有隐式构造函数，
//用返回值智能指针构造了Query对象
inline Query operator~(const Query &operand) {
    return shared_ptr<QueryBase>(new NotQuery(operand));
}

//eval是Query功能的实现
QueryResult NotQuery::eval(const TextQuery &text) const {
    auto result = query.eval(text);
    auto ret_lines = make_shared<set<line_no>>();
    auto beg = result.begin();
    auto end = result.end();
    auto sz = result.get_file()->size();
    for(size_t n = 0; n != sz; ++n) {
        if(beg == end || *beg != n) {
            ret_lines->insert(n);
        } else if (beg != end)
        {
            ++beg;
        }
    }
    return QueryResult(rep(), ret_lines, result.get_file());
}

//BinaryQuery类保存两个运算对象的查询类型所需数据，依然是抽象基类
class BinaryQuery: public QueryBase {
protected:
    BinaryQuery(const Query &l, const Query &r, string s)
    : lhs(l), rhs(r), op_symbol(s) { }
    string rep() const {
        return "(" + lhs.rep() + " " + op_symbol + " " + rhs.rep() + ")";
    }
    Query lhs;
    Query rhs;
    string op_symbol;
};

//AndQuery类
class AndQuery: public BinaryQuery {
    friend Query operator&(const Query &, const Query &);
    AndQuery(const Query &left, const Query &right)
    : BinaryQuery(left, right, "&") { }
    QueryResult eval(const TextQuery &) const override;
};

//创建了AndQuery对象，但使用基类指针进行管理
inline Query operator&(const Query &lhs, const Query &rhs) {
    return shared_ptr<QueryBase>(new AndQuery(lhs, rhs));
}

QueryResult AndQuery::eval(const TextQuery &text) const {
    auto left = lhs.eval(text);
    auto right = rhs.eval(text);
    auto ret_lines = make_shared<set<line_no>>();
    set_intersection(left.begin(), left.end(),
    right.begin(), right.end(), inserter(*ret_lines, ret_lines->begin()));
    return QueryResult(rep(), ret_lines, left.get_file());
}

class OrQuery: public BinaryQuery{
    friend Query operator|(const Query &, const Query &);
    OrQuery(const Query &left, const Query &right)
    : BinaryQuery(left, right, "|") { }
    QueryResult eval(const TextQuery &) const;
};

//创建了OrQuery对象，用基类指针进行管理
inline Query operator|(const Query &lhs, const Query &rhs) {
    return shared_ptr<QueryBase>(new OrQuery(lhs, rhs));
}

QueryResult OrQuery::eval(const TextQuery &text) const {
    auto right = rhs.eval(text);
    auto left = lhs.eval(text);
    auto ret_lines = make_shared<set<line_no>>(left.begin(), left.end());
    ret_lines->insert(right.begin(), right.end());
    return QueryResult(rep(), ret_lines, left.get_file());
}

#endif