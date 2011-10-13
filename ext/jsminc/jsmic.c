#include "ruby.h"

static char theA;
static char theB;
static char theLookahead = '\0';

static char *current_in;
static char *current_out;

/* isAlphanum -- return true if the character is a letter, digit, underscore,
   dollar sign, or non-ASCII character.
   */

static int
isAlphanum(char c)
{
    return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c == '\\' ||
            c > 126);
}


/* get -- return the next character from the string. Watch out for lookahead. If
   the character is a control character, translate it to a space or
   linefeed.
   */

static char
get()
{
    char c = theLookahead;
    theLookahead = '\0';
    if (c == '\0') {
        c = *current_in;
        current_in++;
    }
    if (c >= ' ' || c == '\n' || c == '\0') {
        return c;
    }
    if (c == '\r') {
        return '\n';
    }
    return ' ';
}

static void
write_char(const char c)
{
    *current_out = c;
    current_out++;
}

/* peek -- get the next character without getting it.
   */

static char
peek()
{
    theLookahead = get();
    return theLookahead;
}


/* next -- get the next character, excluding comments. peek() is used to see
   if a '/' is followed by a '/' or '*'.
   */

static char
next()
{
    char c = get();
    if  (c == '/') {
        switch (peek()) {
        case '/':
            for (;;) {
                c = get();
                if (c <= '\n') {
                    return c;
                }
            }
        case '*':
            get();
            for (;;) {
                switch (get()) {
                case '*':
                    if (peek() == '/') {
                        get();
                        return ' ';
                    }
                    break;
                case '\0':
                    rb_raise(rb_eStandardError, "unterminated comment");
                }
            }
        default:
            return c;
        }
    }
    return c;
}


/* action -- do something! What you do is determined by the argument:
   1   Output A. Copy B to A. Get the next B.
   2   Copy B to A. Get the next B. (Delete A).
   3   Get the next B. (Delete B).
   action treats a string as a single character. Wow!
   action recognizes a regular expression if it is preceded by ( or , or =.
   */

static void
action(int d)
{
    switch (d) {
    case 1:
        write_char(theA);
    case 2:
        theA = theB;
        if (theA == '\'' || theA == '"' || theA == '`') {
            for (;;) {
                write_char(theA);
                theA = get();
                if (theA == theB) {
                    break;
                }
                if (theA == '\\') {
                    write_char(theA);
                    theA = get();
                }
                if (theA == '\0') {
                    rb_raise(rb_eStandardError, "unterminated string literal");
                }
            }
        }
    case 3:
        theB = next();
        if (theB == '/' && (theA == '(' || theA == ',' || theA == '=' ||
                    theA == ':' || theA == '[' || theA == '!' ||
                    theA == '&' || theA == '|' || theA == '?' ||
                    theA == '{' || theA == '}' || theA == ';' ||
                    theA == '\n')) {
            write_char(theA);
            write_char(theB);
            for (;;) {
                theA = get();
                if (theA == '[') {
                    for (;;) {
                        write_char(theA);
                        theA = get();
                        if (theA == ']') {
                            break;
                        }
                        if (theA == '\\') {
                            write_char(theA);
                            theA = get();
                        }
                        if (theA == '\0') {
                            rb_raise(rb_eStandardError, "unterminated set in regex literal");
                        }
                    }
                } else if (theA == '/') {
                    break;
                } else if (theA =='\\') {
                    write_char(theA);
                    theA = get();
                }
                if (theA == '\0') {
                    rb_raise(rb_eStandardError, "unterminated regex literal");
                }
                write_char(theA);
            }
            theB = next();
        }
    }
}


/* jsmin -- Copy the input to the output, deleting the characters which are
   insignificant to JavaScript. Comments will be removed. Tabs will be
   replaced with spaces. Carriage returns will be replaced with linefeeds.
   Most spaces and linefeeds will be removed.
   */

static void
jsmin()
{
    theA = '\n';
    action(3);
    while (theA != '\0') {
        switch (theA) {
        case ' ':
            if (isAlphanum(theB)) {
                action(1);
            } else {
                action(2);
            }
            break;
        case '\n':
            switch (theB) {
            case '{':
            case '[':
            case '(':
            case '+':
            case '-':
                action(1);
                break;
            case ' ':
                action(3);
                break;
            default:
                if (isAlphanum(theB)) {
                    action(1);
                } else {
                    action(2);
                }
            }
            break;
        default:
            switch (theB) {
            case ' ':
                if (isAlphanum(theA)) {
                    action(1);
                    break;
                }
                action(3);
                break;
            case '\n':
                switch (theA) {
                case '}':
                case ']':
                case ')':
                case '+':
                case '-':
                case '"':
                case '\'':
                case '`':
                    action(1);
                    break;
                default:
                    if (isAlphanum(theA)) {
                        action(1);
                    } else {
                        action(3);
                    }
                }
                break;
            default:
                action(1);
                break;
            }
        }
    }
    write_char('\0');
}


static VALUE minify(VALUE self, VALUE _in_s) {
    char *in_s = StringValuePtr(_in_s);
    long in_length = strlen(in_s);
    char out_s[in_length + 1];

#ifdef RUBY_19
    VALUE prev_encoding = rb_funcall(_in_s, rb_intern("encoding"), 0);
#endif

    current_in = in_s;
    current_out = out_s;
    jsmin();

#ifdef RUBY_19
    return rb_funcall(rb_str_new2(out_s + 1), rb_intern("force_encoding"), 1, prev_encoding);
#else
    return rb_str_new2(out_s + 1);
#endif
}


void Init_jsminc() {
    VALUE c = rb_cObject;
    c = rb_const_get(c, rb_intern("JSMinC"));

    rb_define_singleton_method(c, "minify", (VALUE(*)(ANYARGS))minify, 1);
}
