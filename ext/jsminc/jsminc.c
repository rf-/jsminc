#include "ruby.h"

typedef struct jsmin_struct {
	char theA;
	char theB;
	char theX;
	char theY;
	char theLookahead;
	char *in;
	char *out;
} jsmin_struct;

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
get(jsmin_struct *s)
{
    char c = s->theLookahead;
    s->theLookahead = '\0';
    if (c == '\0') {
        c = *(s->in)++;
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
write_char(jsmin_struct *s, const char c)
{
    *(s->out) = c;
	*(s->out)++;
}

/* peek -- get the next character without getting it.
   */

static char
peek(jsmin_struct *s)
{
    s->theLookahead = get(s);
    return s->theLookahead;
}


/* next -- get the next character, excluding comments. peek() is used to see
   if a '/' is followed by a '/' or '*'.
   */

static char
next(jsmin_struct *s)
{
    char c = get(s);
    if  (c == '/') {
        switch (peek(s)) {
        case '/':
            for (;;) {
                c = get(s);
                if (c <= '\n') {
                    break;
                }
            }
            break;
        case '*':
            get(s);
            while (c != ' ') {
                switch (get(s)) {
                case '*':
                    if (peek(s) == '/') {
                        get(s);
                        c = ' ';
                    }
                    break;
                case '\0':
                    rb_raise(rb_eStandardError, "unterminated comment");
                }
            }
            break;
        }
    }
    s->theY = s->theX;
    s->theX = c;
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
action(jsmin_struct *s, int d)
{
    switch (d) {
    case 1:
        write_char(s, s->theA);
        if ((s->theY == '\n' || s->theY == ' ') &&
                (s->theA == '+' || s->theA == '-' || s->theA == '*' || s->theA == '/') &&
                (s->theB == '+' || s->theB == '-' || s->theB == '*' || s->theB == '/')) {
            write_char(s, s->theY);
        }
    case 2:
        s->theA = s->theB;
        if (s->theA == '\'' || s->theA == '"' || s->theA == '`') {
            for (;;) {
                write_char(s, s->theA);
                s->theA = get(s);
                if (s->theA == s->theB) {
                    break;
                }
                if (s->theA == '\\') {
                    write_char(s, s->theA);
                    s->theA = get(s);
                }
                if (s->theA == '\0') {
                    rb_raise(rb_eStandardError, "unterminated string literal");
                }
            }
        }
    case 3:
        s->theB = next(s);
        if (s->theB == '/' && (
                    s->theA == '(' || s->theA == ',' || s->theA == '=' || s->theA == ':' ||
                    s->theA == '[' || s->theA == '!' || s->theA == '&' || s->theA == '|' ||
                    s->theA == '?' || s->theA == '+' || s->theA == '-' || s->theA == '~' ||
                    s->theA == '*' || s->theA == '/' || s->theA == '{' || s->theA == '\n')) {
            write_char(s, s->theA);
            if (s->theA == '/' || s->theA == '*') {
                write_char(s, ' ');
            }
            write_char(s, s->theB);
            for (;;) {
                s->theA = get(s);
                if (s->theA == '[') {
                    for (;;) {
                        write_char(s, s->theA);
                        s->theA = get(s);
                        if (s->theA == ']') {
                            break;
                        }
                        if (s->theA == '\\') {
                            write_char(s, s->theA);
                            s->theA = get(s);
                        }
                        if (s->theA == '\0') {
                            rb_raise(rb_eStandardError, "unterminated set in regex literal");
                        }
                    }
                } else if (s->theA == '/') {
                    switch (peek(s)) {
                    case '/':
                    case '*':
                        rb_raise(rb_eStandardError, "unterminated set in regex literal");
                    }
                    break;
                } else if (s->theA =='\\') {
                    write_char(s, s->theA);
                    s->theA = get(s);
                }
                if (s->theA == '\0') {
                    rb_raise(rb_eStandardError, "unterminated regex literal");
                }
                write_char(s, s->theA);
            }
            s->theB = next(s);
        }
    }
}


/* jsmin -- Copy the input to the output, deleting the characters which are
   insignificant to JavaScript. Comments will be removed. Tabs will be
   replaced with spaces. Carriage returns will be replaced with linefeeds.
   Most spaces and linefeeds will be removed.
   */

static void
jsmin(jsmin_struct *s)
{
    if (peek(s) == 0xEF) {
        get(s);
        get(s);
        get(s);
    }
    s->theA = '\n';

    action(s, 3);
    while (s->theA != '\0') {
        switch (s->theA) {
        case ' ':
            action(s, isAlphanum(s->theB) ? 1 : 2);
            break;
        case '\n':
            switch (s->theB) {
            case '{':
            case '[':
            case '(':
            case '+':
            case '-':
            case '!':
            case '~':
                action(s, 1);
                break;
            case ' ':
                action(s, 3);
                break;
            default:
                action(s, isAlphanum(s->theB) ? 1 : 2);
            }
            break;
        default:
            switch (s->theB) {
            case ' ':
                action(s, isAlphanum(s->theA) ? 1 : 3);
                break;
            case '\n':
                switch (s->theA) {
                case '}':
                case ']':
                case ')':
                case '+':
                case '-':
                case '"':
                case '\'':
                case '`':
                    action(s, 1);
                    break;
                default:
                    action(s, isAlphanum(s->theA) ? 1 : 3);
                }
                break;
            default:
                action(s, 1);
                break;
            }
        }
    }
    write_char(s, '\0');
}


static VALUE minify(VALUE self, VALUE _in_s) {
    jsmin_struct s;

    char *in_s = StringValueCStr(_in_s);
    long in_length = strlen(in_s);
    char out_s[in_length + 1];

#ifdef RUBY_19
    VALUE prev_encoding = rb_funcall(_in_s, rb_intern("encoding"), 0);
#endif

    s.theLookahead = '\0';
    s.theX = '\0';
    s.theY = '\0';
    s.in = in_s;
    s.out = out_s;
    jsmin(&s);

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
