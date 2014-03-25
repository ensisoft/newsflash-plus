
// $Id: unit_test_string.cpp,v 1.6 2009/11/15 14:56:36 svaisane Exp $

#include <boost/test/minimal.hpp>
#include <boost/algorithm/string.hpp>
#include "../string.h"
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>

using namespace std;
using namespace stringlib;


void generate_test_strings(vector<string>& search, vector<string>& keys, int count)
{
    enum { SEARCH_LEN = 100 };

    const char* alphabet = "abcdefghijlkmnopqrstuvxyz";
    size_t len = strlen(alphabet);

    string s;
    s.resize(SEARCH_LEN);

    for (int i=0; i<count; ++i)
    {
        for (int x=0; x<SEARCH_LEN; ++x)
            s[x] = alphabet[rand() % len];

        search.push_back(s);
            
        // take a chunk of the search string to for key
        string key;
        int start_pos = rand() % (SEARCH_LEN - 16);
        int end_pos   = start_pos + (rand() % 15) + 1;
        
        BOOST_REQUIRE(start_pos < end_pos);
        
        string::iterator start = s.begin();
        string::iterator end   = s.begin();

        advance(start, start_pos);
        advance(end, end_pos);
        BOOST_REQUIRE(end > start);
        std::copy(start, end, back_inserter(key));
            
        keys.push_back(key);
        
        if (!(i % 1000))
        {
            cout << "generating test data. patience....\n";
            cout.flush();
        }
    }
}

/*
 * Synopsis: Test matching at different parts of the haystack 
 *
 * Expected: match is found
 */
void test0()
{

    BOOST_REQUIRE(find_match("this is a test of the boyer moore algorithm", "algorithm"));
    BOOST_REQUIRE(find_match("abbaxxxxxx", "abba"));
    BOOST_REQUIRE(find_match("xxxabbaxxx", "abba"));
    BOOST_REQUIRE(find_match("xxxxxxabba", "abba"));
    BOOST_REQUIRE(find_match("aaaaaaabba", "abba"));
    BOOST_REQUIRE(find_match("abba", "abba"));
    BOOST_REQUIRE(find_match("ababbaabba", "abba"));
    BOOST_REQUIRE(find_match("aaaa", "aaa"));
    BOOST_REQUIRE(find_match("aaa", "aaaa")==false);
    BOOST_REQUIRE(find_match("", "aaa") == false);
    BOOST_REQUIRE(find_match(NULL, "") == false);

    BOOST_REQUIRE(find_match("cpojhcbrdcgfiesdnhbmaprqunaotvtvolgyojqrozxzfreszgfbvatspxjjudgjpoigabaoczoiqsbszjvxkqsbpcklitxalhhl", "otvtv"));
    BOOST_REQUIRE(find_match("eezfmpdbfjgralgdhrtdxazbramhdfjjjioybuzigfbgrhlabgfzhfazfpglxpvhbmhejhmqonxjxjjzqoaytczbrinpajzdxghi", "h"));
    BOOST_REQUIRE(find_match("tasigjydhuqhintigvvddjlgecgejgdeivoohproliyuyteescivkuepxmuhsandyerftkvhvvcuqibkkkhxgnpgalnvmdzlhtpe", "bkkk"));
    BOOST_REQUIRE(find_match("zeraxknsppjhnrfikfvfiyevppcsaaeaeyadkqvbgfltarfkybqgcxerogkpgrpmqrpclogqvqnvjsljvbrybvqpcehkvxaorqtc", "brybvqpc"));

    BOOST_REQUIRE(find_match("zbryberaxknsppjbrybhnrfikfvfiyevppbrybcsaaeaeyabrybdbryöööäääää#¤&#1!!1234567890sabasdgascabsg", "ööää"));
    BOOST_REQUIRE(find_match("zbryberaxknsppjbrybhnrfikfvfiyevppbrybcsaaeaeyabrybdbryöööäääää#¤&#1!!1234567890sabasdgascabsg", "1234"));

    BOOST_REQUIRE(find_match("asdgasascagdwgas\0220\0221\0222\0223asdgasgas", "\0220\0221\0222"));

    BOOST_REQUIRE(find_match("zeraxknsppjhnrfikfvfiyevppcsaaeaeyadkqvbgfltarfkybqgcxerogkpgrpmqrpclogqvqnvjsljvehkvxaorqtcbrybvqpc", "brybvqpc"));
    BOOST_REQUIRE(find_match("zeraxknsppjhnrfikfvfiyebrybvqpcvppcsaaeaeyadkqvbgfltarfkybqgcxerogkpgrpmqrpclogqvqnvjsljvehkvxaorqtc", "brybvqpc"));
    BOOST_REQUIRE(find_match("brybvqzeraxknsppjhnbrybvqpcrfikfvfiyevppcsaaeaeyadkqvbgfltarfkybqgcxerogkpgrpmqrpclogqvqnvjsljvehkvxaorqtc", "brybvqpc"));
    
    BOOST_REQUIRE(find_match("bbrbrybrybbrybvbrybvqbrybvqpbrybvqpc", "brybvqpc"));
}

/*
 * Synopsis: Test almost match and no match.
 *
 * Expected: no match
 */
void test1()
{
    BOOST_REQUIRE(!find_match("\" ForU-4All \"1\"\"La Soupe Aux Choux Techno Remix 2005.mp3\" yEnc (01/19)", "jallukola"));
    BOOST_REQUIRE(!find_match("\"Die Schlmpfe Vol.15.rar\"Die Schlmpfe vol. 15 - mit cover - by Ktzchen /Kater (001/266)\'Yenc@power-post.org (by Ktzchen/Kater)", "jallukola"));
    BOOST_REQUIRE(!find_match("\"70s 80s  - Funky town.mp3\" yEnc-Doc Snyder Postet (1/8)'Yenc@power-post.org (Doc Snyder Postet)", "jallukola"));
    
    BOOST_REQUIRE(!find_match("brybvqpczeraxknsppjhnrfikfvfiyevppcsaaeaeyadkqvbgfltarfkybqgcxerogkpgrpmqrpclogqvqnvjsljvehkvxaorqtc", "brybvqpcc"));
    BOOST_REQUIRE(!find_match("zeraxknsppjhnrfikfvfiyevppcsaaeaeyadkqvbgfltbrybvqpcarfkybqgcxerogkpgrpmqrpclogqvqnvjsljvehkvxaorqtc", "brybvqpcc"));
    BOOST_REQUIRE(!find_match("zeraxknsppjhnrfikfvfiyevppcsaaeaeyadkqvbgfltarfkybqgcxerogkpgrpmqrpclogqvqnvjsljvehkvxaorqtcbrybvqp", "brybvqpcc"));

    BOOST_REQUIRE(!find_match("zbryberaxknsppjbrybhnrfikfvfiyevppbrybcsaaeaeyabrybdbrybbrybbrybkqvbgfltbrybarfkybqgcxerogkpgrpmqrpclogqvqnvjsljvehkvxaorqtcbryb", "brybvqpc"));
    
    BOOST_REQUIRE(!find_match("(????) [05/60] - \"105 - Jacqueline Bach - Trume.mp3\" yEnc (01/22) Yenc@power-post.org (Radio Herz)", "metallica"));
    BOOST_REQUIRE(!find_match("\"Atomik Harmonik.rar\"Atomik Harmonik- ( by Ktzchen&Kater ) (001/178)'Yenc@power-post.org (by Ktzchen/Kater)", "metallica"));

}

void test2()
{
    vector<string> search;
    vector<string> key;
    
    const int iterations = 10000;

    search.reserve(iterations);
    key.reserve(iterations);

    generate_test_strings(search, key, iterations);
    
    BOOST_REQUIRE(search.size() == iterations);
    BOOST_REQUIRE(key.size() == iterations);
    
    for (int i=0; i<iterations; ++i)
    {
        const string& s = search[i];
        const string& k=  key[i];
        
        bool found = find_match(s, k);
        if (!found)
        {
            cout << "iteration: " << i << endl;
            cout << "search: " << s << endl;
            cout << "key: "    << k << endl;
            cout << "\n\n";
        }
        BOOST_REQUIRE(found);

        if (!(i % 1000))
        {
            cout << "working....";
            cout.flush();
        }
        
        // comparison functions
        ::strstr(s.c_str(), k.c_str());
        boost::find_first(search, key);
    }

}

void test3()
{
    {
        string_matcher<> m("otvtv");
        BOOST_REQUIRE(!m.empty());
        BOOST_REQUIRE(m.search("cpojhcbrdcgfiesdnhbmaprqunaotvtvolgyojqrozxzfreszgfbvatspxjjudgjpoigabaoczoiqsbszjvxkqsbpcklitxalhhl"));
    }
    {
        string_matcher<> m("jallukola");
        BOOST_REQUIRE(!m.search("\"70s 80s  - Funky town.mp3\" yEnc-Doc Snyder Postet (1/8)'Yenc@power-post.org (Doc Snyder Postet)"));
    }
    {
        string_matcher<> m("otvtv");
        std::string haystack("cpojhcbrdcgfiesdnhbmaprqunaotvtvolgyojqrozxzfreszgfbvatspxjjudgjpoigabaoczoiqsbszjvxkqsbpcklitxalhhl");
        BOOST_REQUIRE(m.search(haystack));
    }
    {
        std::string haystack("assa sassa mandelmassa");
        std::string needle("ssa");
        BOOST_REQUIRE(find_match(haystack, needle));
    }
    {
        string_matcher<> m("FooBaR", false);
        BOOST_REQUIRE(m.search("fagaszgasgasaFooBaRasgsag"));
        
    }
    {
        string_matcher<> m("FoOBaR", false);
        BOOST_REQUIRE(m.search("asdgasfgasffoobar"));
    }
    
    {
        string_matcher<> m("f\0344\0344BaR", false);
        BOOST_REQUIRE(m.search("fasgaszgasgfqaswqetqwQWETQzaadz124352af\0344\0344BaR"));
        
    }
    
    {
        string_matcher<> m("METALLICA", false);
        BOOST_REQUIRE(m.search("German TOP 100 19.09.2008  056/110J- \"056_Metallica_-_Nothing_Else_Matters.mp3\" yEnc (02/44)"));
    }
    
}


void test4()
{
    vector<string> search;
    vector<string> key;
    
    const int iterations = 10000;
    
    search.reserve(iterations);
    key.reserve(iterations);
    
    generate_test_strings(search, key, iterations);
    
    BOOST_REQUIRE(search.size() == iterations);
    BOOST_REQUIRE(key.size() == iterations);
    
    string foobar("foobar");

    for (int i=0; i<iterations; ++i)
    {
        const string& s = search[i];
        
        find_match(search[i], foobar);
    }
}


void test5()
{
    std::vector<int> v;
    v.push_back(3);
    v.push_back(30);
    v.push_back(23);
    v.push_back(88);
    v.push_back(12);
    v.push_back(1);
    v.push_back(2);
    v.push_back(99);
    v.push_back(1344);
    
    std::vector<int> v2;
    v2.push_back(1);
    v2.push_back(2);
    v2.push_back(99);
    
    BOOST_REQUIRE(find_match(v, v2));
}

void test6()
{
    BOOST_REQUIRE(find_longest_common_substring("", "") == "");
    BOOST_REQUIRE(find_longest_common_substring("foo", "bar") == "");
    BOOST_REQUIRE(find_longest_common_substring("foobar", "foo") == "foo");
    BOOST_REQUIRE(find_longest_common_substring("foobar", "bar") == "bar");
    BOOST_REQUIRE(find_longest_common_substring("fooasdgsaxcgasfoobar", "foobar") == "foobar");
    BOOST_REQUIRE(find_longest_common_substring("foobar\0223\0223keke", "aa\0223\0223k") == "\0223\0223k");
    BOOST_REQUIRE(find_longest_common_substring("asgasgasgasFOOBARasdgasfga", "FOOBAasasdga", false) == "FOOBA");
    
}

int test_main(int argc, char* argv[])
{
    test0();
    test1();
    //test2();
    test3();
    test4();
    
    //test5();
    
    test6();

    return 0;
}
