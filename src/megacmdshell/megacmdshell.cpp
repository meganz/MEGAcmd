/**
 * @file src/megacmdshell.cpp
 * @brief MEGAcmd: Interactive CLI and service application
 * This is the shell application
 *
 * (c) 2013-2017 by Mega Limited, Auckland, New Zealand
 *
 * This file is distributed under the terms of the GNU General Public
 * License, see http://www.gnu.org/copyleft/gpl.txt
 * for details.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "megacmdshell.h"
#include "megacmdshellcommunications.h"
#include "megacmdshellcommunicationsnamedpipes.h"

#define USE_VARARGS
#define PREFER_STDARG

#ifdef NO_READLINE
#include <megaconsole.h>
#else
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include <iomanip>
#include <string>
#include <cstring>
#include <set>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>
#include <stdio.h>

#define PROGRESS_COMPLETE -2
#define SPROGRESS_COMPLETE "-2"
#define PROMPT_MAX_SIZE 128

#ifndef _WIN32
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#else
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#ifndef _O_U16TEXT
#define _O_U16TEXT 0x00020000
#endif
#ifndef _O_U8TEXT
#define _O_U8TEXT 0x00040000
#endif
#endif

#if defined(_WIN32)
  #define strdup _strdup
#endif

#define SSTR( x ) static_cast< const std::ostringstream & >( \
        (  std::ostringstream() << std::dec << x ) ).str()

using namespace std;
using namespace mega;

#if defined(NO_READLINE) && defined(_WIN32)
CONSOLE_CLASS* console = NULL;
#endif


// utility functions
char * dupstr(char* s)
{
    char *r;

    r = (char*)malloc(sizeof( char ) * ( strlen(s) + 1 ));
    strcpy(r, s);
    return( r );
}

void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty())
    {
        return;
    }
    size_t start_pos = 0;
    while (( start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

#ifndef NO_READLINE
string getCurrentLine()
{
    char *saved_line = rl_copy_text(0, rl_point);
    string toret(saved_line);
    free(saved_line);
    saved_line = NULL;
    return toret;
}
#endif

void sleepSeconds(int seconds)
{
#ifdef _WIN32
    Sleep(1000*seconds);
#else
    sleep(seconds);
#endif
}

void sleepMilliSeconds(long milliseconds)
{
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds *1000);
#endif
}

vector<string> getlistOfWords(const char *ptr, bool ignoreTrailingSpaces = true)
{
    vector<string> words;

    const char* wptr;

    // split line into words with quoting and escaping
    for (;; )
    {
        // skip leading blank space
        while (*(const signed char*)ptr > 0 && *ptr <= ' ' && (ignoreTrailingSpaces || *(ptr+1)))
        {
            ptr++;
        }

        if (!*ptr)
        {
            break;
        }

        // quoted arg / regular arg
        if (*ptr == '"')
        {
            ptr++;
            wptr = ptr;
            words.push_back(string());

            for (;; )
            {
                if (( *ptr == '"' ) || ( *ptr == '\\' ) || !*ptr)
                {
                    words[words.size() - 1].append(wptr, ptr - wptr);

                    if (!*ptr || ( *ptr++ == '"' ))
                    {
                        break;
                    }

                    wptr = ptr - 1;
                }
                else
                {
                    ptr++;
                }
            }
        }
        else if (*ptr == '\'') // quoted arg / regular arg
        {
            ptr++;
            wptr = ptr;
            words.push_back(string());

            for (;; )
            {
                if (( *ptr == '\'' ) || ( *ptr == '\\' ) || !*ptr)
                {
                    words[words.size() - 1].append(wptr, ptr - wptr);

                    if (!*ptr || ( *ptr++ == '\'' ))
                    {
                        break;
                    }

                    wptr = ptr - 1;
                }
                else
                {
                    ptr++;
                }
            }
        }
        else
        {
            while (*ptr == ' ') ptr++;// only possible if ptr+1 is the end

            wptr = ptr;

            const char *prev = ptr;
            //while ((unsigned char)*ptr > ' ')
            while ((*ptr != '\0') && !(*ptr ==' ' && *prev !='\\'))
            {
                if (*ptr == '"')
                {
                    while (*++ptr != '"' && *ptr != '\0')
                    { }
                }
                prev=ptr;
                ptr++;
            }

            words.push_back(string(wptr, ptr - wptr));
        }
    }

    return words;
}

void discardOptionsAndFlags(vector<string> *ws)
{
    for (std::vector<string>::iterator it = ws->begin(); it != ws->end(); )
    {
        /* std::cout << *it; ... */
        string w = ( string ) * it;
        if (w.length() && ( w.at(0) == '-' )) //begins with "-"
        {
            it = ws->erase(it);
        }
        else //not an option/flag
        {
            ++it;
        }
    }
}


// end utily functions

string clientID; //identifier for a registered state listener

long long charstoll(const char *instr)
{
  long long retval;

  retval = 0;
  for (; *instr; instr++) {
    retval = 10*retval + (*instr - '0');
  }
  return retval;
}

// Console related functions:
void console_readpwchar(char* pw_buf, int pw_buf_size, int* pw_buf_pos, char** line)
{
#ifdef _WIN32
    char c;
      DWORD cread;

      if (ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &c, 1, &cread, NULL) == 1)
      {
          if ((c == 8) && *pw_buf_pos)
          {
              (*pw_buf_pos)--;
          }
          else if (c == 13)
          {
              *line = (char*)malloc(*pw_buf_pos + 1);
              memcpy(*line, pw_buf, *pw_buf_pos);
              (*line)[*pw_buf_pos] = 0;
          }
          else if (*pw_buf_pos < pw_buf_size)
          {
              pw_buf[(*pw_buf_pos)++] = c;
          }
      }
#else
    // FIXME: UTF-8 compatibility

    char c;

    if (read(STDIN_FILENO, &c, 1) == 1)
    {
        if (c == 8 && *pw_buf_pos)
        {
            (*pw_buf_pos)--;
        }
        else if (c == 13)
        {
            *line = (char*) malloc(*pw_buf_pos + 1);
            memcpy(*line, pw_buf, *pw_buf_pos);
            (*line)[*pw_buf_pos] = 0;
        }
        else if (*pw_buf_pos < pw_buf_size)
        {
            pw_buf[(*pw_buf_pos)++] = c;
        }
    }
#endif
}
void console_setecho(bool echo)
{
#ifdef _WIN32
    HANDLE hCon = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;

    GetConsoleMode(hCon, &mode);

    if (echo)
    {
        mode |= ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT;
    }
    else
    {
        mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    }

    SetConsoleMode(hCon, mode);
#else
    //do nth
#endif
}

int getNumberOfCols(unsigned int defaultwidth=0)
{
    unsigned int width = defaultwidth;
    int rows = 1, cols = width;
#ifdef NO_READLINE
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    BOOL ok = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbi);
    assert(ok);
    if (ok)
    {
        cols = sbi.dwSize.X ;
    }
#elif defined( RL_ISSTATE ) && defined( RL_STATE_INITIALIZED )

    if (RL_ISSTATE(RL_STATE_INITIALIZED))
    {
        rl_resize_terminal();
        rl_get_screen_size(&rows, &cols);
    }
#endif

    if (cols)
    {
        width = cols-2;
#ifdef _WIN32
        width--;
#endif
    }
    return width;
}
bool alreadyFinished = false; //flag to show progress
float percentDowloaded = 0.0; // to show progress


// password change-related state information
string oldpasswd;
string newpasswd;

bool doExit = false;
bool doReboot = false;
bool handlerinstalled = false;

bool requirepromptinstall = true;

bool procesingline = false;


static char dynamicprompt[PROMPT_MAX_SIZE];

static char* line;

static prompttype prompt = COMMAND;

static char pw_buf[256];
static int pw_buf_pos = 0;

string loginname;
bool signingup = false;
string signupline;
string passwdline;
string linktoconfirm;

bool confirminglink = false;
bool confirmingcancellink = false;

// communications with megacmdserver:
MegaCmdShellCommunications *comms;

MUTEX_CLASS mutexPrompt(false);

void printWelcomeMsg(unsigned int width = 0);



void statechangehandle(string statestring)
{
    char statedelim[2]={(char)0x1F,'\0'};
    size_t nextstatedelimitpos = statestring.find(statedelim);

    while (nextstatedelimitpos!=string::npos && statestring.size())
    {
        string newstate = statestring.substr(0,nextstatedelimitpos);
        statestring=statestring.substr(nextstatedelimitpos+1);
        nextstatedelimitpos = statestring.find(statedelim);
        if (newstate.compare(0, strlen("prompt:"), "prompt:") == 0)
        {
            changeprompt(newstate.substr(strlen("prompt:")).c_str(),true);
        }
        else if (newstate.compare(0, strlen("message:"), "message:") == 0)
        {
            OUTSTREAM << endl << newstate.substr(strlen("message:")) << endl;
        }
        else if (newstate.compare(0, strlen("clientID:"), "clientID:") == 0)
        {
            clientID = newstate.substr(strlen("clientID:")).c_str();
        }
        else if (newstate.compare(0, strlen("progress:"), "progress:") == 0)
        {
            string rest = newstate.substr(strlen("progress:"));

            size_t nexdel = rest.find(":");
            string received = rest.substr(0,nexdel);

            rest = rest.substr(nexdel+1);
            nexdel = rest.find(":");
            string total = rest.substr(0,nexdel);

            string title;
            if ( (nexdel != string::npos) && (nexdel < rest.size() ) )
            {
                rest = rest.substr(nexdel+1);
                nexdel = rest.find(":");
                title = rest.substr(0,nexdel);
            }

            if (title.size())
            {
                if (received==SPROGRESS_COMPLETE)
                {
                    printprogress(PROGRESS_COMPLETE, charstoll(total.c_str()),title.c_str());

                }
                else
                {
                    printprogress(charstoll(received.c_str()), charstoll(total.c_str()),title.c_str());
                }
            }
            else
            {
                if (received==SPROGRESS_COMPLETE)
                {
                    printprogress(PROGRESS_COMPLETE, charstoll(total.c_str()));
                }
                else
                {
                    printprogress(charstoll(received.c_str()), charstoll(total.c_str()));
                }
            }

        }
        else if (newstate == "ack")
        {
            // do nothing, all good
        }
        else if (newstate == "restart")
        {
            doExit = true;
            doReboot = true;
            comms->updating = true; // to avoid mensajes about server down
            sleepSeconds(3); // Give a while for server to restart
            changeprompt("RESTART REQUIRED BY SERVER (due to an update). Press any key to continue.", true);
        }
        else
        {
            cerr << "received unrecognized state change: [" << newstate << "]" << endl;
            //sleep a while to avoid continuous looping
            sleepSeconds(1);
        }
    }
}


void sigint_handler(int signum)
{
    if (prompt != COMMAND)
    {
        setprompt(COMMAND);
    }

#ifndef NO_READLINE
    // reset position and print prompt
    rl_replace_line("", 0); //clean contents of actual command
    rl_crlf(); //move to nextline

    if (RL_ISSTATE(RL_STATE_ISEARCH) || RL_ISSTATE(RL_STATE_ISEARCH) || RL_ISSTATE(RL_STATE_ISEARCH))
    {
        RL_UNSETSTATE(RL_STATE_ISEARCH);
        RL_UNSETSTATE(RL_STATE_NSEARCH);
        RL_UNSETSTATE( RL_STATE_SEARCH);
        history_set_pos(history_length);
        rl_restore_prompt(); // readline has stored it when searching
    }
    else
    {
        rl_reset_line_state();
    }
    rl_redisplay();
#endif
}


void printprogress(long long completed, long long total, const char *title)
{
    int cols = getNumberOfCols(80);

    string outputString;
    outputString.resize(cols + 1);
    for (int i = 0; i < cols; i++)
    {
        outputString[i] = '.';
    }

    outputString[cols] = '\0';
    char *ptr = (char *)outputString.c_str();
    sprintf(ptr, "%s%s", title, " ||");
    ptr += strlen(title);
    ptr += strlen(" ||");
    *ptr = '.'; //replace \0 char

    float oldpercent = percentDowloaded;
    if (total == 0)
    {
        percentDowloaded = 0;
    }
    else
    {
        percentDowloaded = float(completed * 1.0 / total * 100.0);
    }
    if (completed != PROGRESS_COMPLETE && (alreadyFinished || ( ( percentDowloaded == oldpercent ) && ( oldpercent != 0 ) ) ))
    {
        return;
    }
    if (percentDowloaded < 0)
    {
        percentDowloaded = 0;
    }

    char aux[41];
    if (total < 0)
    {
        return; // after a 100% this happens
    }
    if (completed != PROGRESS_COMPLETE  && completed < 0.001 * total)
    {
        return; // after a 100% this happens
    }
    if (completed == PROGRESS_COMPLETE)
    {
        alreadyFinished = true;
        completed = total;
        percentDowloaded = 100;
    }
    sprintf(aux,"||(%lld/%lld MB: %6.2f %%) ", completed / 1024 / 1024, total / 1024 / 1024, percentDowloaded);
    sprintf((char *)outputString.c_str() + cols - strlen(aux), "%s",                         aux);
    for (int i = 0; i < ( cols - (strlen(title) + strlen(" ||")) - strlen(aux)) * 1.0 * min(100.0f,percentDowloaded) / 100.0; i++)
    {
        *ptr++ = '#';
    }

    if (alreadyFinished)
    {
        cerr << outputString << endl;
    }
    else
    {
        cerr << outputString << '\r' << flush;
    }

}


#ifdef _WIN32
BOOL WINAPI CtrlHandler( DWORD fdwCtrlType )
{
  cerr << "Reached CtrlHandler: " << fdwCtrlType << endl;

  switch( fdwCtrlType )
  {
    // Handle the CTRL-C signal.
    case CTRL_C_EVENT:
       sigint_handler((int)fdwCtrlType);
      return( TRUE );

    default:
      return FALSE;
  }
}
#endif

prompttype getprompt()
{
    return prompt;
}

void setprompt(prompttype p, string arg)
{
    prompt = p;

#ifndef NO_READLINE
    if (p == COMMAND)
    {
        console_setecho(true);
    }
    else
    {
        pw_buf_pos = 0;
        if (arg.size())
        {
            OUTSTREAM << arg << flush;
        }
        else
        {
            OUTSTREAM << prompts[p] << flush;
        }
        console_setecho(false);
    }
#else
    console->setecho(p == COMMAND);
    
    if (p != COMMAND)
    {
        pw_buf_pos = 0;
        console->updateInputPrompt(arg.empty() ? prompts[p] : arg);
    }
#endif
}

#ifndef NO_READLINE
// readline callback - exit if EOF, add to history unless password
static void store_line(char* l)
{
    procesingline = true;
    if (!l)
    {
#ifndef _WIN32 // to prevent exit with Supr key
        doExit = true;
        rl_set_prompt("(CTRL+D) Exiting ...\n");
#ifndef NDEBUG
        if (comms->serverinitiatedfromshell)
        {
            OUTSTREAM << " Forwarding exit command to the server, since this cmd shell (most likely) initiated it" << endl;
            comms->executeCommand("exit", readresponse);
        }
#endif
#endif
        return;
    }

    if (*l && ( prompt == COMMAND ))
    {
        add_history(l);
    }

    line = l;
}
#endif

#ifdef _WIN32

bool validoptionforreadline(const string& string)
{// TODO: this has not been tested in 100% cases (perhaps it is too diligent or too strict)
    int c,i,ix,n,j;
    for (i=0, ix=int(string.length()); i < ix; i++)
    {
        c = (unsigned char) string[i];

        //if (c>0xC0) return false;
        //if (c==0x09 || c==0x0a || c==0x0d || (0x20 <= c && c <= 0x7e) ) n = 0; // is_printable_ascii
        if (0x00 <= c && c <= 0x7f) n=0; // 0bbbbbbb
        else if ((c & 0xE0) == 0xC0) n=1; // 110bbbbb
        else if ( c==0xed && i<(ix-1) && ((unsigned char)string[i+1] & 0xa0)==0xa0) return false; //U+d800 to U+dfff
        else if ((c & 0xF0) == 0xE0) {return false; n=2;} // 1110bbbb
        else if ((c & 0xF8) == 0xF0) {return false; n=3;} // 11110bbb
        //else if (($c & 0xFC) == 0xF8) n=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
        //else if (($c & 0xFE) == 0xFC) n=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
        else return false;
        for (j=0; j<n && i<ix; j++) { // n bytes matching 10bbbbbb follow ?
            if ((++i == ix) || (( (unsigned char)string[i] & 0xC0) != 0x80))
                return false;
        }
    }
    return true;
}

bool validwcharforeadline(const wchar_t thewchar)
{
    wstring input;
    input+=thewchar;
    string output;
    localwtostring(&input,&output);
    return validoptionforreadline(output);
}

wstring escapereadlinebreakers(const wchar_t *what)
{
    wstring output;
    for( unsigned int i = 0; i < wcslen( what ) ; i++ )
    {
        if(validwcharforeadline(what[ i ] ))
        {
            output.reserve( output.size() + 1 );
            output += what[ i ];
        } else {
#ifndef __MINGW32__
            wchar_t code[ 7 ];
            swprintf( code, 7, L"\\u%0.4X", what[ i ] ); //while this does not work (yet) as what, at least it shows something and does not break
            //TODO: ideally we would do the conversion from escaped unicode chars \uXXXX back to wchar_t in the server
            // NOTICE: I was able to execute a command with a literl \x242ee (which correspond to \uD850\uDEEE in UTF16).
            // So it'll be more interesting to output here the complete unicode char and in unescapeutf16escapedseqs revert it.
            //     or keep here the UTF16 escaped secs and revert them correctly in the unescapeutf16escapedseqs
            output.reserve( output.size() + 7 ); // "\u"(2) + 5(uint max digits capacity)
            output += code;
#endif
        }
    }
    return output;
}
#endif

#ifndef NO_READLINE
void install_rl_handler(const char *theprompt)
{
#ifdef _WIN32
    wstring wswhat;
    stringtolocalw(theprompt,&wswhat);
    const wchar_t *what = wswhat.c_str();


    // escape characters that break readline input (e.g. Chinese ones. e.g \x242ee)
    wstring output = escapereadlinebreakers(what);

    // give readline something it understands
    what = output.c_str();
    size_t buffer_size;
#ifdef _TRUNCATE
    wcstombs_s(&buffer_size, NULL, 0, what, _TRUNCATE);
#else
    buffer_size=output.size()*sizeof(wchar_t)*2;
#endif

    if (buffer_size) //coversion is ok
    {
        // do the actual conversion
        char *buffer = new char[buffer_size];
        #ifdef _TRUNCATE
            wcstombs_s(&buffer_size, buffer, buffer_size,what, _TRUNCATE);
        #else
            wcstombs(buffer, what, buffer_size);
        #endif

        rl_callback_handler_install(buffer, store_line);
    }
    else
    {
        rl_callback_handler_install("INVALID_PROMPT: ", store_line);
    }

#else
    rl_callback_handler_install(theprompt, store_line);
#endif
}
#endif

void changeprompt(const char *newprompt, bool redisplay)
{
    MutexGuard g(mutexPrompt);

    if (*dynamicprompt)
    {
        if (!strcmp(newprompt,dynamicprompt))
            return; //same prompt. do nth
    }

    strncpy(dynamicprompt, newprompt, sizeof( dynamicprompt ));

    if (strlen(newprompt) >= PROMPT_MAX_SIZE)
    {
        strncpy(dynamicprompt, newprompt, PROMPT_MAX_SIZE/2-1);
        dynamicprompt[PROMPT_MAX_SIZE/2-1] = '.';
        dynamicprompt[PROMPT_MAX_SIZE/2] = '.';

        strncpy(dynamicprompt+PROMPT_MAX_SIZE/2+1, newprompt+(strlen(newprompt)-PROMPT_MAX_SIZE/2+2), PROMPT_MAX_SIZE/2-2);
        dynamicprompt[PROMPT_MAX_SIZE-1] = '\0';
    }

#ifdef NO_READLINE
    console->updateInputPrompt(newprompt);
#else
    if (redisplay)
    {
        // save line
        int saved_point = rl_point;
        char *saved_line = rl_copy_text(0, rl_end);

        rl_clear_message();

        // enter a new line if not processing sth (otherwise, the newline should already be there)
        if (!procesingline)
        {
            rl_crlf();
        }

        install_rl_handler(*dynamicprompt ? dynamicprompt : prompts[COMMAND]);

        // restore line
        if (saved_line)
        {
            rl_replace_line(saved_line, 0);
            free(saved_line);
            saved_line = NULL;
        }
        rl_point = saved_point;
        rl_redisplay();

        handlerinstalled = true;

        requirepromptinstall = false;
    }

#endif

    static bool firstime = true;
    if (firstime)
    {
        firstime = false;
#if _WIN32
        if( !SetConsoleCtrlHandler( CtrlHandler, TRUE ) )
        {
            cerr << "Control handler set failed" << endl;
        }
#else
        // prevent CTRL+C exit
        signal(SIGINT, sigint_handler);
#endif
    }
}

void escapeEspace(string &orig)
{
    replaceAll(orig," ", "\\ ");
}

void unescapeEspace(string &orig)
{
    replaceAll(orig,"\\ ", " ");
}

#ifndef NO_READLINE
char* empty_completion(const char* text, int state)
{
    // we offer 2 different options so that it doesn't complete (no space is inserted)
    if (state == 0)
    {
        return strdup(" ");
    }
    if (state == 1)
    {
        return strdup(text);
    }
    return NULL;
}

char* generic_completion(const char* text, int state, vector<string> validOptions)
{
    static size_t list_index, len;
    static bool foundone;
    string name;
    if (!validOptions.size()) // no matches
    {
        return empty_completion(text,state); //dont fall back to filenames
    }
    if (!state)
    {
        list_index = 0;
        foundone = false;
        len = strlen(text);
    }
    while (list_index < validOptions.size())
    {
        name = validOptions.at(list_index);
        if (!rl_completion_quote_character) {
            escapeEspace(name);
        }

        list_index++;

        if (!( strcmp(text, "")) || (( name.size() >= len ) && ( strlen(text) >= len ) && ( name.find(text) == 0 )))
        {
            if (name.size() && (( name.at(name.size() - 1) == '=' ) || ( name.at(name.size() - 1) == '\\' ) || ( name.at(name.size() - 1) == '/' )))
            {
                rl_completion_suppress_append = 1;
            }
            foundone = true;
            return dupstr((char*)name.c_str());
        }
    }

    if (!foundone)
    {
        return empty_completion(text,state); //dont fall back to filenames
    }

    return((char*)NULL );
}
#endif


inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

char* local_completion(const char* text, int state)
{
    return((char*)NULL );  //matches will be NULL: readline will use local completion
}

void pushvalidoption(vector<string>  *validOptions, const char *beginopt)
{
#ifdef _WIN32
    if (validoptionforreadline(beginopt))
    {
        validOptions->push_back(beginopt);
    }
    else
    {
        wstring input;
        stringtolocalw(beginopt,&input);
        wstring output = escapereadlinebreakers(input.c_str());

        string soutput;
        localwtostring(&output,&soutput);
        validOptions->push_back(soutput.c_str());
    }
#else
    validOptions->push_back(beginopt);
#endif
}

void changedir(const string& where)
{
#ifdef _WIN32
    wstring wwhere;
    stringtolocalw(where.c_str(), &wwhere);
    wwhere.append(L"\\");
    int r = SetCurrentDirectoryW((LPCWSTR)wwhere.data());
    if (!r)
    {
        cerr << "Error at SetCurrentDirectoryW before local completion to " << where << ". errno: " << ERRNO << endl;
    }
#else
    chdir(where.c_str());
#endif
}


#ifndef NO_READLINE
char* remote_completion(const char* text, int state)
{
    char *saved_line = strdup(getCurrentLine().c_str());

    static vector<string> validOptions;
    if (state == 0)
    {
        validOptions.clear();
        string completioncommand("completionshell ");
        completioncommand+=saved_line;

        OUTSTRING s;
        OUTSTRINGSTREAM oss(s);

        comms->executeCommand(completioncommand, readresponse, oss);

        string outputcommand;

#ifdef _WIN32
        #ifdef __MINGW32__
            wstring soss=oss.str();
            localwtostring(&soss,&outputcommand);
        #else
            localwtostring(&oss.str(),&outputcommand);
        #endif
#else
         outputcommand = oss.str();
#endif

        if (outputcommand == "MEGACMD_USE_LOCAL_COMPLETION")
        {
            free(saved_line);
            return local_completion(text,state); //fallback to local path completion
        }

        if (outputcommand.find("MEGACMD_USE_LOCAL_COMPLETION") == 0)
        {
            string where = outputcommand.substr(strlen("MEGACMD_USE_LOCAL_COMPLETION"));
            changedir(where);
            free(saved_line);
            return local_completion(text,state); //fallback to local path completion
        }

        char *ptr = (char *)outputcommand.c_str();

        char *beginopt = ptr;
        while (*ptr)
        {
            if (*ptr == 0x1F)
            {
                *ptr = '\0';
                if (strcmp(beginopt," ")) //the server will give a " " for empty_completion (no matches)
                {
                    pushvalidoption(&validOptions,beginopt);
                }

                beginopt=ptr+1;
            }
            ptr++;
        }
        if (*beginopt && strcmp(beginopt," "))
        {
            pushvalidoption(&validOptions,beginopt);
        }
    }

    free(saved_line);
    saved_line = NULL;

    return generic_completion(text, state, validOptions);
}

static char** getCompletionMatches(const char * text, int start, int end)
{
    rl_filename_quoting_desired = 1;

    char **matches;

    matches = (char**)NULL;

    matches = rl_completion_matches((char*)text, remote_completion);

    return( matches );
}

void printHistory()
{
    int length = history_length;
    int offset = 1;
    int rest = length;
    while (rest >= 10)
    {
        offset++;
        rest = rest / 10;
    }

    for (int i = 0; i < length; i++)
    {
        history_set_pos(i);
        OUTSTREAM << setw(offset) << i << "  " << current_history()->line << endl;
    }
}

#ifdef _WIN32

/**
 * @brief getcharacterreadlineUTF16support
 * while this works, somehow arrows and other readline stuff is disabled using this one.
 * @param stream
 * @return
 */
int getcharacterreadlineUTF16support (FILE *stream)
{
    int result;
    char b[10];
    memset(b,0,10);

    while (1)
    {
        int oldmode = _setmode(_fileno(stream), _O_U16TEXT);

        result = read (fileno (stream), &b, 10);
        _setmode(_fileno(stream), oldmode);

        if (result == 0)
        {
            return (EOF);
        }

        // convert the UTF16 string to widechar
        size_t wbuffer_size;
#ifdef _TRUNCATE
        mbstowcs_s(&wbuffer_size, NULL, 0, b, _TRUNCATE);
#else
        wbuffer_size=10;
#endif
        wchar_t *wbuffer = new wchar_t[wbuffer_size];

#ifdef _TRUNCATE
        mbstowcs_s(&wbuffer_size, wbuffer, wbuffer_size, b, _TRUNCATE);
#else
        mbstowcs(wbuffer, b, wbuffer_size);
#endif

        // convert the UTF16 widechar to UTF8 string
        string receivedutf8;
        utf16ToUtf8(wbuffer, wbuffer_size,&receivedutf8);

        if (strlen(receivedutf8.c_str()) > 1) //multi byte utf8 sequence: place the UTF8 characters into rl buffer one by one
        {
            for (unsigned int i=0;i< strlen(receivedutf8.c_str());i++)
            {
                rl_line_buffer[rl_end++] = receivedutf8.c_str()[i];
                rl_point=rl_end;
            }
            rl_line_buffer[rl_end] = '\0';

            return 0;
        }

        if (result =! 0)
        {
            return (b[0]);
        }

        /* If zero characters are returned, then the file that we are
     reading from is empty!  Return EOF in that case. */
        if (result == 0)
        {
            return (EOF);
        }
    }
}
#endif

void wait_for_input(int readline_fd)
{
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(readline_fd, &fds);

    int rc = select(FD_SETSIZE, &fds, NULL, NULL, NULL);
    if (rc < 0)
    {
        if (ERRNO != EINTR)  //syscall
        {
#ifdef _WIN32
         if (ERRNO != WSAENOTSOCK) // it enters here since it is not a socket. Alt: Use WaitForMultipleObjectsEx
#endif
                cerr << "Error at select at wait_for_input errno: " << ERRNO << endl;
            return;
        }
    }
}
#else


vector<autocomplete::ACState::Completion> remote_completion(string linetocomplete)
{
    using namespace autocomplete;
    vector<ACState::Completion> result;

    // normalize any partially or intermediately quoted strings, eg.  `put c:\Program" Fi` or `/My" Documents/"`
    ACState acs = prepACState(linetocomplete, linetocomplete.size(), console->getAutocompleteStyle());
    string refactoredline;
    for (auto& s : acs.words)
    {
        refactoredline += (refactoredline.empty() ? "" : " ") + s.getQuoted();
    }

    OUTSTRING s;
    OUTSTRINGSTREAM oss(s);
    comms->executeCommand(string("completionshell ") + refactoredline, readresponse, oss);

    string outputcommand;
    localwtostring(&oss.str(), &outputcommand);

    ACState::quoted_word completionword = acs.words.size() ? acs.words[acs.words.size() - 1] : string();

    if (outputcommand.find("MEGACMD_USE_LOCAL_COMPLETION") == 0)
    {
        string where = outputcommand.substr(strlen("MEGACMD_USE_LOCAL_COMPLETION"));
        changedir(where);

        if (acs.words.size())
        {
            string l = completionword.getQuoted();
            CompletionState cs = autoComplete(l, l.size(), acs.words[0].s == "lcd" ? localFSFolder() : localFSPath(), console->getAutocompleteStyle());
            result.swap(cs.completions);
        }
        return result;
    }
    else
    {
        char *ptr = (char *)outputcommand.c_str();

        char *beginopt = ptr;
        while (*ptr)
        {
            if (*ptr == 0x1F)
            {
                *ptr = '\0';
                if (strcmp(beginopt, " ")) //the server will give a " " for empty_completion (no matches)
                {
                    result.push_back(autocomplete::ACState::Completion(beginopt, false));
                }

                beginopt = ptr + 1;
            }
            ptr++;
        }
        if (*beginopt && strcmp(beginopt, " "))
        {
            result.push_back(autocomplete::ACState::Completion(beginopt, false));
        }

        if (result.size() == 1 && result[0].s == completionword.s)
        {
            result.clear();  // for parameters it returns the same string when there are no matches
        }
        return result;
    }
}

void exec_clear(autocomplete::ACState& s)
{
    console->clearScreen();
}

void exec_history(autocomplete::ACState& s)
{
    console->outputHistory();
}

void exec_dos_unix(autocomplete::ACState& s)
{
    if (s.words.size() < 2)
    {
        OUTSTREAM << "autocomplete style: " << (console->getAutocompleteStyle() ? "unix" : "dos") << endl;
    }
    else
    {
        console->setAutocompleteStyle(s.words[1].s == "unix");
    }
}

void exec_codepage(autocomplete::ACState& s)
{
    if (s.words.size() == 1)
    {
        UINT cp1, cp2;
        console->getShellCodepages(cp1, cp2);
        cout << "Current codepage is " << cp1;
        if (cp2 != cp1)
        {
            cout << " with failover to codepage " << cp2 << " for any absent glyphs";
        }
        cout << endl;
        for (int i = 32; i < 256; ++i)
        {
            string theCharUtf8 = WinConsole::toUtf8String(WinConsole::toUtf16String(string(1, (char)i), cp1));
            cout << "  dec/" << i << " hex/" << hex << i << dec << ": '" << theCharUtf8 << "'";
            if (i % 4 == 3)
            {
                cout << endl;
            }
        }
    }
    else if (s.words.size() == 2 && atoi(s.words[1].s.c_str()) != 0)
    {
        if (!console->setShellConsole(atoi(s.words[1].s.c_str()), atoi(s.words[1].s.c_str())))
        {
            cout << "Code page change failed - unicode selected" << endl;
        }
    }
    else if (s.words.size() == 3 && atoi(s.words[1].s.c_str()) != 0 && atoi(s.words[2].s.c_str()) != 0)
    {
        if (!console->setShellConsole(atoi(s.words[1].s.c_str()), atoi(s.words[2].s.c_str())))
        {
            cout << "Code page change failed - unicode selected" << endl;
        }
    }
    else
    {
        cout << "      codepage [N [N]]" << endl;
    }
}


autocomplete::ACN autocompleteSyntax;

autocomplete::ACN buildAutocompleteSyntax()
{
    using namespace autocomplete;
    std::unique_ptr<Either> p(new Either("      "));

    p->Add(exec_clear,      sequence(text("clear")));
    p->Add(exec_codepage,   sequence(text("codepage"), opt(sequence(wholenumber(65001), opt(wholenumber(65001))))));
    p->Add(exec_dos_unix,   sequence(text("autocomplete"), opt(either(text("unix"), text("dos")))));
    p->Add(exec_history,    sequence(text("history")));

    return autocompleteSyntax = std::move(p);
}

void printHistory()
{
    console->outputHistory();
}
#endif

bool isserverloggedin()
{
    if (comms->executeCommand(("loggedin")) == MCMD_NOTLOGGEDIN )
    {
        return false;
    }
    return true;
}


void process_line(const char * line)
{
    string refactoredline;

    switch (prompt)
    {
        case AREYOUSURE:
            //this is currently never used
            if (!strcasecmp(line,"yes") || !strcasecmp(line,"y"))
            {
                comms->setResponseConfirmation(true);
                setprompt(COMMAND);
            }
            else if (!strcasecmp(line,"no") || !strcasecmp(line,"n"))
            {
                comms->setResponseConfirmation(false);
                setprompt(COMMAND);
            }
            else
            {
                //Do nth, ask again
                OUTSTREAM << "Please enter: [y]es/[n]o: " << flush;
            }
            break;
        case LOGINPASSWORD:
        {
            if (!strlen(line))
            {
                break;
            }
            if (confirminglink)
            {
                string confirmcommand("confirm ");
                confirmcommand+=linktoconfirm;
                confirmcommand+=" " ;
                confirmcommand+=loginname;
                confirmcommand+=" " ;
                confirmcommand+=line;
                OUTSTREAM << endl;
                comms->executeCommand(confirmcommand.c_str(), readresponse);
            }
            else if (confirmingcancellink)
            {
                string confirmcommand("confirmcancel ");
                confirmcommand+=linktoconfirm;
                confirmcommand+=" " ;
                confirmcommand+=line;
                OUTSTREAM << endl;
                comms->executeCommand(confirmcommand.c_str(), readresponse);
            }
            else
            {
                string logincommand("login -v ");
                if (clientID.size())
                {
                    logincommand += "--clientID=";
                    logincommand+=clientID;
                    logincommand+=" ";
                }
                logincommand+=loginname;
                logincommand+=" " ;
                logincommand+=line;
                OUTSTREAM << endl;
                comms->executeCommand(logincommand.c_str(), readresponse);
            }
            confirminglink = false;
            confirmingcancellink = false;

            setprompt(COMMAND);
            break;
        }
        case NEWPASSWORD:
        {
            if (!strlen(line))
            {
                break;
            }
            newpasswd = line;
            OUTSTREAM << endl;
            setprompt(PASSWORDCONFIRM);
            break;
        }
        case PASSWORDCONFIRM:
        {
            if (!strlen(line))
            {
                break;
            }
            if (line != newpasswd)
            {
                OUTSTREAM << endl << "New passwords differ, please try again" << endl;
            }
            else
            {
                OUTSTREAM << endl;

                if (signingup)
                {
                    signupline += " ";
                    signupline += newpasswd;
                    comms->executeCommand(signupline.c_str(), readresponse);

                    signingup = false;
                }
                else
                {
                    string changepasscommand(passwdline);
                    passwdline = " ";
                    changepasscommand+=oldpasswd;
                    changepasscommand+=" " ;
                    changepasscommand+=newpasswd;
                    comms->executeCommand(changepasscommand.c_str(), readresponse);
                }
            }

            setprompt(COMMAND);
            break;
        }
        case COMMAND:
        {

#ifdef NO_READLINE
            // if local command and syntax is satisfied, execute it
            string consoleOutput;
            if (autocomplete::autoExec(line, string::npos, autocompleteSyntax, false, consoleOutput, false))
            {
                COUT << consoleOutput << flush;
                return;
            }

            // normalize any partially or intermediately quoted strings, eg.  `put c:\Program" Fi` or get `/My" Documents/"`
            autocomplete::ACState acs = autocomplete::prepACState(line, strlen(line), console->getAutocompleteStyle());
            for (auto& s : acs.words)
            {
                refactoredline += (refactoredline.empty() ? "" : " ") + s.getQuoted();
            }
            line = refactoredline.c_str();
#endif

            vector<string> words = getlistOfWords(line);
            bool helprequested = false;
            for (unsigned int i = 1; i< words.size(); i++)
            {
                if (words[i]== "--help") helprequested = true;
            }
            if (words.size())
            {
                if ( words[0] == "exit" || words[0] == "quit")
                {
                    if (words.size() == 1)
                    {
                        doExit = true;
                    }
                    if (words.size() == 1 || words[1]!="--only-shell")
                    {
                        comms->executeCommand(line, readresponse);
                    }
                    else
                    {
                        doExit = true;
                    }
                }
#ifndef __linux__
                else if (words[0] == "update")
                {
                    OUTSTREAM << "Updating ..." << endl;

                    MegaCmdShellCommunications::updating = true;
                    int ret = comms->executeCommand(line, readresponse);
                    if (ret == MCMD_REQRESTART)
                    {
                        OUTSTREAM << "MEGAcmd has been updated ... this shell will be restarted before proceding...." << endl;
                        doExit = true;
                        doReboot = true;
                    }
                    else if (ret != MCMD_INVALIDSTATE)
                    {
                        MegaCmdShellCommunications::updating = false;
                        OUTSTREAM << "Update is not required. You are in the last version. Further info: \"version --help\"" << endl;
                    }
                }
#endif
                else if (words[0] == "history")
                {
                    if (helprequested)
                    {
                        OUTSTREAM << " Prints commands history" << endl;
                    }
                    else
                    {
                        printHistory();
                    }
                }
#if defined(_WIN32) && !defined(NO_READLINE)
                else if (!helprequested && words[0] == "unicode" && words.size() == 1)
                {
                    rl_getc_function=(rl_getc_function==&getcharacterreadlineUTF16support)?rl_getc:&getcharacterreadlineUTF16support;
                    OUTSTREAM << "Unicode shell input " << ((rl_getc_function==&getcharacterreadlineUTF16support)?"ENABLED":"DISABLED") << endl;
                    return;
                }
#endif
                else if (!helprequested && words[0] == "passwd")
                {
                    if (isserverloggedin())
                    {
                        passwdline = line;
                        discardOptionsAndFlags(&words);
                        if (words.size() == 1)
                        {
                            setprompt(NEWPASSWORD);
                        }
                        else
                        {
                            comms->executeCommand(line, readresponse);
                        }
                    }
                    else
                    {
                        cerr << "Not logged in." << endl;
                    }

                    return;
                }
                else if (!helprequested && words[0] == "login")
                {
                    if (!isserverloggedin())
                    {
                        discardOptionsAndFlags(&words);

                        if (words.size() == 2 && (words[1].find("@") != string::npos))
                        {
                            loginname = words[1];
                            setprompt(LOGINPASSWORD);
                        }
                        else
                        {
                            string s = line;
                            if (clientID.size())
                            {
                                s = "login --clientID=";
                                s+=clientID;
                                s.append(string(line).substr(5));
                            }
                            comms->executeCommand(s, readresponse);
                        }
                    }
                    else
                    {
                        cerr << "Already logged in. Please log out first." << endl;
                    }
                    return;
                }
                else if (!helprequested && words[0] == "signup")
                {
                    if (!isserverloggedin())
                    {
                        signupline = line;
                        discardOptionsAndFlags(&words);

                        if (words.size() == 2)
                        {
                            loginname = words[1];
                            signingup = true;
                            setprompt(NEWPASSWORD);
                        }
                        else
                        {
                            comms->executeCommand(line, readresponse);
                        }
                    }
                    else
                    {
                        cerr << "Please loggout first." << endl;
                    }
                    return;
                }
                else if (!helprequested && words[0] == "confirm")
                {
                    discardOptionsAndFlags(&words);

                    if (words.size() == 3)
                    {
                        linktoconfirm = words[1];
                        loginname = words[2];
                        confirminglink = true;
                        setprompt(LOGINPASSWORD);
                    }
                    else
                    {
                        comms->executeCommand(line, readresponse);
                    }
                }
                else if (!helprequested && words[0] == "confirmcancel")
                {
                    discardOptionsAndFlags(&words);

                    if (words.size() == 2)
                    {
                        linktoconfirm = words[1];
                        confirmingcancellink = true;
                        setprompt(LOGINPASSWORD);
                    }
                    else
                    {
                        comms->executeCommand(line, readresponse);
                    }
                    return;
                }
                else if (!helprequested && words[0] == "clear")
                {
#ifdef _WIN32
                    HANDLE hStdOut;
                    CONSOLE_SCREEN_BUFFER_INFO csbi;
                    DWORD count;

                    hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
                    if (hStdOut == INVALID_HANDLE_VALUE) return;

                    /* Get the number of cells in the current buffer */
                    if (!GetConsoleScreenBufferInfo( hStdOut, &csbi )) return;
                    /* Fill the entire buffer with spaces */
                    if (!FillConsoleOutputCharacter( hStdOut, (TCHAR) ' ', csbi.dwSize.X *csbi.dwSize.Y, { 0, 0 }, &count ))
                    {
                        return;
                    }
                    /* Fill the entire buffer with the current colors and attributes */
                    if (!FillConsoleOutputAttribute(hStdOut, csbi.wAttributes, csbi.dwSize.X *csbi.dwSize.Y, { 0, 0 }, &count))
                    {
                        return;
                    }
                    /* Move the cursor home */
                    SetConsoleCursorPosition( hStdOut, { 0, 0 } );
#else
                    rl_clear_screen(0,0);
#endif
                    return;
                }
                else if ( (words[0] == "transfers"))
                {
                    string toexec;

                    if (!strstr (line,"path-display-size"))
                    {
                        unsigned int width = getNumberOfCols(75);
                        int pathSize = int((width-46)/2);

                        toexec+=words[0];
                        toexec+=" --path-display-size=";
                        toexec+=SSTR(pathSize);
                        toexec+=" ";
                        if (strlen(line)>(words[0].size()+1))
                        {
                            toexec+=line+words[0].size()+1;
                        }
                    }
                    else
                    {
                        toexec+=line;
                    }

                    comms->executeCommand(toexec.c_str(), readresponse);
                }
                else if ( (words[0] == "du"))
                {
                    string toexec;

                    if (!strstr (line,"path-display-size"))
                    {
                        unsigned int width = getNumberOfCols(75);
                        int pathSize = int(width-13);

                        toexec+=words[0];
                        toexec+=" --path-display-size=";
                        toexec+=SSTR(pathSize);
                        toexec+=" ";
                        if (strlen(line)>(words[0].size()+1))
                        {
                            toexec+=line+words[0].size()+1;
                        }
                    }
                    else
                    {
                        toexec+=line;
                    }

                    comms->executeCommand(toexec.c_str(), readresponse);
                }
                else if (words[0] == "sync")
                {
                    string toexec;

                    if (!strstr (line,"path-display-size"))
                    {
                        unsigned int width = getNumberOfCols(75);
                        int pathSize = int((width-46)/2);

                        toexec+="sync --path-display-size=";
                        toexec+=SSTR(pathSize);
                        toexec+=" ";
                        if (strlen(line)>strlen("sync "))
                        {
                            toexec+=line+strlen("sync ");
                        }
                    }
                    else
                    {
                        toexec+=line;
                    }

                    comms->executeCommand(toexec.c_str(), readresponse);
                }
                else if (words[0] == "backup")
                {
                    string toexec;

                    if (!strstr (line,"path-display-size"))
                    {
                        unsigned int width = getNumberOfCols(75);
                        int pathSize = int((width-21)/2);

                        toexec+="backup --path-display-size=";
                        toexec+=SSTR(pathSize);
                        toexec+=" ";
                        if (strlen(line)>strlen("backup "))
                        {
                            toexec+=line+strlen("backup ");
                        }
                    }
                    else
                    {
                        toexec+=line;
                    }

                    comms->executeCommand(toexec.c_str(), readresponse);
                }
                else
                {
                    if ( words[0] == "get" || words[0] == "put" || words[0] == "reload")
                    {
                        string s = line;
                        if (clientID.size())
                        {
                            string sline = line;
                            size_t pspace = sline.find_first_of(" ");
                            s="";
                            s=sline.substr(0,pspace);
                            s += " --clientID=";
                            s+=clientID;
                            if (pspace!=string::npos)
                            {
                                s+=sline.substr(pspace);
                            }
                            words.push_back(s);
                        }
                        comms->executeCommand(s, readresponse);
#ifdef _WIN32
                        Sleep(200); // give a brief while to print progress ended
#endif
                    }
                    else
                    {
                        // execute user command
                        comms->executeCommand(line, readresponse);
                    }
                }
            }
            else
            {
                cerr << "failed to interprete input line: " << line << endl;
            }
            break;
        }
    }

}

// main loop
#ifndef NO_READLINE
void readloop()
{
    time_t lasttimeretrycons = 0;

    char *saved_line = NULL;
    int saved_point = 0;

    rl_save_prompt();

    int readline_fd = -1;

    readline_fd = fileno(rl_instream);

    comms->registerForStateChanges(statechangehandle);

    //give it a while to communicate the state
    sleepMilliSeconds(700);

#if defined(_WIN32) && defined(USE_PORT_COMMS)
    // due to a failure in reconnecting to the socket, if the server was initiated in while registeringForStateChanges
    // in windows we would not be yet connected. we need to manually try to register again.
    if (comms->registerAgainRequired)
    {
        comms->registerForStateChanges(statechangehandle);
    }
    //give it a while to communicate the state
    sleepMilliSeconds(1);
#endif

    for (;; )
    {
        if (prompt == COMMAND)
        {
            mutexPrompt.lock();
            if (requirepromptinstall)
            {
                install_rl_handler(*dynamicprompt ? dynamicprompt : prompts[COMMAND]);

                handlerinstalled = false;

                // display prompt
                if (saved_line)
                {
                    rl_replace_line(saved_line, 0);
                    free(saved_line);
                    saved_line = NULL;
                }

                rl_point = saved_point;
                rl_redisplay();
            }
            mutexPrompt.unlock();
        }



        // command editing loop - exits when a line is submitted
        for (;; )
        {
            if (prompt == COMMAND || prompt == AREYOUSURE)
            {
                procesingline = false;

                wait_for_input(readline_fd);

                rl_callback_read_char(); //this calls store_line if last char was enter

                time_t tnow = time(NULL);
                if ( (tnow - lasttimeretrycons) > 5 && !doExit && !MegaCmdShellCommunications::updating)
                {
                    comms->executeCommand("retrycons");
                    lasttimeretrycons = tnow;
                }

                rl_resize_terminal(); // to always adjust to new screen sizes

                if (doExit)
                {
                    if (saved_line != NULL)
                        free(saved_line);
                    saved_line = NULL;
                    return;
                }
            }
            else
            {
                console_readpwchar(pw_buf, sizeof pw_buf, &pw_buf_pos, &line);
            }

            if (line)
            {
                break;
            }
        }

        // save line
        saved_point = rl_point;
        if (saved_line != NULL)
            free(saved_line);
        saved_line = rl_copy_text(0, rl_end);

        // remove prompt
        rl_save_prompt();
        rl_replace_line("", 0);
        rl_redisplay();

        if (line)
        {
            if (strlen(line))
            {
                alreadyFinished = false;
                percentDowloaded = 0.0;
                mutexPrompt.lock();
                process_line(line);
                requirepromptinstall = true;
                mutexPrompt.unlock();

                if (comms->registerAgainRequired)
                {
                    // register again for state changes
                     comms->registerForStateChanges(statechangehandle);
                     comms->registerAgainRequired = false;
                }

                // sleep, so that in case there was a changeprompt waiting, gets executed before relooping
                // this is not 100% guaranteed to happen
                sleepSeconds(0);
            }
            free(line);
            line = NULL;
        }
        if (doExit)
        {
            if (saved_line != NULL)
                free(saved_line);
            saved_line = NULL;
            return;
        }
    }
}
#else  // NO_READLINE
void readloop()
{
    time_t lasttimeretrycons = 0;

    comms->registerForStateChanges(statechangehandle);

    //give it a while to communicate the state
    sleepMilliSeconds(700);

#if defined(_WIN32) && defined(USE_PORT_COMMS)
    // due to a failure in reconnecting to the socket, if the server was initiated in while registeringForStateChanges
    // in windows we would not be yet connected. we need to manually try to register again.
    if (comms->registerAgainRequired)
    {
        comms->registerForStateChanges(statechangehandle);
    }
    //give it a while to communicate the state
    sleepMilliSeconds(1);
#endif

    for (;; )
    {
        if (prompt == COMMAND)
        {
            console->updateInputPrompt(*dynamicprompt ? dynamicprompt : prompts[COMMAND]);
        }

        // command editing loop - exits when a line is submitted
        for (;; )
        {
            line = console->checkForCompletedInputLine();

            if (line)
            {
                break;
            }
            else
            {
                time_t tnow = time(NULL);
                if ((tnow - lasttimeretrycons) > 5 && !doExit && !MegaCmdShellCommunications::updating)
                {
                    comms->executeCommand("retrycons");
                    lasttimeretrycons = tnow;
                }

                if (doExit)
                {
                    return;
                }
            }
        }

        if (line)
        {
            if (strlen(line))
            {
                alreadyFinished = false;
                percentDowloaded = 0.0;
                mutexPrompt.lock();
                process_line(line);
                requirepromptinstall = true;
                mutexPrompt.unlock();

                if (comms->registerAgainRequired)
                {
                    // register again for state changes
                    comms->registerForStateChanges(statechangehandle);
                    comms->registerAgainRequired = false;
                }

                // sleep, so that in case there was a changeprompt waiting, gets executed before relooping
                // this is not 100% guaranteed to happen
                sleepSeconds(0);
            }
            free(line);
            line = NULL;
        }
        if (doExit)
        {
            return;
        }
    }
}
#endif

class NullBuffer : public std::streambuf
{
public:
    int overflow(int c)
    {
        return c;
    }
};

void printCenteredLine(string msj, unsigned int width, bool encapsulated = true)
{
    if (msj.size()>width)
    {
        width = unsigned(msj.size());
    }
    if (encapsulated)
        COUT << "|";
    for (unsigned int i = 0; i < (width-msj.size())/2; i++)
        COUT << " ";
    COUT << msj;
    for (unsigned int i = 0; i < (width-msj.size())/2 + (width-msj.size())%2 ; i++)
        COUT << " ";
    if (encapsulated)
        COUT << "|";
    COUT << endl;
}

void printWelcomeMsg(unsigned int width)
{
    if (!width)
    {
        width = getNumberOfCols(75);
    }

    COUT << endl;
    COUT << ".";
    for (unsigned int i = 0; i < width; i++)
        COUT << "=" ;
    COUT << ".";
    COUT << endl;
    printCenteredLine(" __  __ _____ ____    _                      _ ",width);
    printCenteredLine("|  \\/  | ___|/ ___|  / \\   ___ _ __ ___   __| |",width);
    printCenteredLine("| |\\/| | \\  / |  _  / _ \\ / __| '_ ` _ \\ / _` |",width);
    printCenteredLine("| |  | | /__\\ |_| |/ ___ \\ (__| | | | | | (_| |",width);
    printCenteredLine("|_|  |_|____|\\____/_/   \\_\\___|_| |_| |_|\\__,_|",width);

    COUT << "|";
    for (unsigned int i = 0; i < width; i++)
        COUT << " " ;
    COUT << "|";
    COUT << endl;
    printCenteredLine("Welcome to MEGAcmd! A Command Line Interactive and Scriptable",width);
    printCenteredLine("Application to interact with your MEGA account.",width);
    printCenteredLine("Please write to support@mega.nz if you find any issue or",width);
    printCenteredLine("have any suggestion concerning its functionalities.",width);
    printCenteredLine("Enter \"help --non-interactive\" to learn how to use MEGAcmd with scripts.",width);
    printCenteredLine("Enter \"help\" for basic info and a list of available commands.",width);

#if defined(_WIN32) && defined(NO_READLINE)
    printCenteredLine("Unicode support in the console is improved, see \"help --unicode\"", width);
#elif defined(_WIN32)
    printCenteredLine("Enter \"help --unicode\" for info regarding non-ASCII support.",width);
#endif

    COUT << "`";
    for (unsigned int i = 0; i < width; i++)
        COUT << "=" ;
    COUT << "´";
    COUT << endl;

}

int quote_detector(char *line, int index)
{
    return (
        index > 0 &&
        line[index - 1] == '\\' &&
        !quote_detector(line, index - 1)
    );
}


bool runningInBackground()
{
#ifndef _WIN32
    pid_t fg = tcgetpgrp(STDIN_FILENO);
    if(fg == -1) {
        // Piped:
        return false;
    }  else if (fg == getpgrp()) {
        // foreground
        return false;
    } else {
        // background
        return true;
    }
#endif
    return false;
}

#ifdef _WIN32
void mycompletefunct(char **c, int num_matches, int max_length)
{
    int rows = 1, cols = 80;

#if defined( RL_ISSTATE ) && defined( RL_STATE_INITIALIZED )

            if (RL_ISSTATE(RL_STATE_INITIALIZED))
            {
                rl_resize_terminal();
                rl_get_screen_size(&rows, &cols);
            }
#endif


    // max_length is not trustworthy
    for (int i=1; i <= num_matches; i++) //contrary to what the documentation says, num_matches is not the size of c (but num_matches+1), current text is preappended in c[0]
    {
        max_length = max(max_length,(int)strlen(c[i]));
    }

    OUTSTREAM << endl;

    int nelements_per_col = max(1,(cols-1)/(max_length+1));
    for (int i=1; i <= num_matches; i++) //contrary to what the documentation says, num_matches is not the size of c (but num_matches+1), current text is preappended in c[0]
    {
        string option = c[i];

        OUTSTREAM << setw(min(cols-1,max_length+1)) << left;
        int oldmode = _setmode(_fileno(stdout), _O_U16TEXT);
        OUTSTREAM << c[i];
        _setmode(_fileno(stdout), oldmode);

        if ( (i%nelements_per_col == 0) && (i != num_matches))
        {
            OUTSTREAM << endl;
        }
    }
    OUTSTREAM << endl;
}
#endif

#ifndef NO_READLINE
std::string readresponse(const char* question)
{
    string response;
    response = readline(question);
    rl_set_prompt("");
    rl_replace_line("", 0);
    return response;
}
#else
std::string readresponse(const char* question)
{
    COUT << question << flush;
    console->updateInputPrompt(question);
    for (;;)
    {
        if (char* line = console->checkForCompletedInputLine())
        {
            console->updateInputPrompt("");
            string response(line);
            free(line);
            return response;
        }
        else
        {
            sleepMilliSeconds(200);
        }
    }
}
#endif


int main(int argc, char* argv[])
{

#if defined(_WIN32) && !defined(NO_READLINE)
    // Set Environment's default locale
    setlocale(LC_ALL, "en-US");
    rl_completion_display_matches_hook = mycompletefunct;
#endif

    // intialize the comms object
#if defined(_WIN32) && !defined(USE_PORT_COMMS)
    comms = new MegaCmdShellCommunicationsNamedPipes();
#else
    comms = new MegaCmdShellCommunications();
#endif

#ifndef NO_READLINE
    rl_attempted_completion_function = getCompletionMatches;
    rl_completer_quote_characters = "\"'";
    rl_filename_quote_characters  = " ";
    rl_completer_word_break_characters = (char *)" ";

    rl_char_is_quoted_p = &quote_detector;

    if (!runningInBackground())
    {
        rl_initialize(); // initializes readline,
        // so that we can use rl_message or rl_resize_terminal safely before ever
        // prompting anything.
    }
#endif

#if defined(_WIN32) && defined(NO_READLINE)
    console = new CONSOLE_CLASS;
    console->setAutocompleteSyntax(buildAutocompleteSyntax());
    console->setAutocompleteFunction(remote_completion);
    console->setShellConsole(CP_UTF8, GetConsoleOutputCP());
    console->blockingConsolePeek = true;
#endif

#ifdef _WIN32
    // in windows, rl_resize_terminal fails to resize before first prompt appears, we take the width from elsewhere
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left - 2;
    printWelcomeMsg(columns);
#else
    sleepMilliSeconds(200); // this gives a little while so that the console is ready and rl_resize_terminal works fine
    printWelcomeMsg();
#endif

    readloop();

#ifndef NO_READLINE
    clear_history();
    rl_callback_handler_remove(); //To avoid having the terminal messed up (requiring a "reset")
#endif
    delete comms;

    if (doReboot)
    {
#ifdef __WIN32
        sleepSeconds(5); // Give a while for server to restart
        LPWSTR szPathExecQuoted = GetCommandLineW();
        wstring wspathexec = wstring(szPathExecQuoted);

        if (wspathexec.at(0) == '"')
        {
            wspathexec = wspathexec.substr(1);
        }
        while (wspathexec.size() && ( wspathexec.at(wspathexec.size()-1) == '"' || wspathexec.at(wspathexec.size()-1) == ' ' ))
        {
            wspathexec = wspathexec.substr(0,wspathexec.size()-1);
        }
        LPWSTR szPathExec = (LPWSTR) wspathexec.c_str();

        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory( &si, sizeof(si) );
        ZeroMemory( &pi, sizeof(pi) );
        si.cb = sizeof(si);

        if (!CreateProcess( szPathExec,szPathExec,NULL,NULL,TRUE,
                            CREATE_NEW_CONSOLE,
                            NULL,NULL,
                            &si,&pi) )
        {
            COUT << "Unable to execute: " << szPathExec << " errno = : " << ERRNO << endl;
            sleepSeconds(5);
        }
#else
        execv(argv[0], argv);
#endif

    }
}
