// TextColor.h
#ifndef __TEXTCOLOR_H__
#define __TEXTCOLOR_H__

#define CSI "\x1B["

// printf("\x1B[32mHello") prints Hello in green
// but kinda washed out looking "\x1b[1m" = BOLD
// (combined as printf("\x1B32;1mHELLO") makes a nice readable green HELLO
// be sure to FORE_NORMAL at the end or text will stay the color

// ex: printf(FORE_RED"Error doing something!\n"FORE_NORMAL);
//  -- prints the error in RED.
//
// NEW and IMPROVED: Just call _RED("Error doing something!"); or _GREEN("Hello");
//  These auto-flush (so you DON'T need flush(NULL) anymore) and adds the newline (\n) for you.
// NEW NEW NEW NEW!! 
#define TEXT_GRAY CSI"30;1m"
#define TEXT_RED CSI"31;1m"
#define TEXT_GREEN CSI"32;1m"
#define TEXT_YELLOW CSI"33;1m"
// Dark bright blue = 34;1, background CYAN = 46:
#define TEXT_BLUE CSI"34;1;46m"
#define TEXT_MAGENTA CSI"35;1m"
#define TEXT_CYAN CSI"36;1m"
#define TEXT_WHITE CSI"37;1m"

#define TEXT_NORMAL CSI"0m"

#ifdef __cplusplus
#define _RED(s) do { \
	std::cout << TEXT_RED << (s) << TEXT_NORMAL << std::endl; \
	} while(false)

#define _GREEN(s) do { \
	std::cout << TEXT_GREEN << (s) << TEXT_NORMAL << std::endl; \
	} while(false)

#define _YELLOW(s) do { \
	std::cout << TEXT_YELLOW << (s) << TEXT_NORMAL << std::endl; \
	} while(false)

#define _BLUE(s) do { \
	std::cout << TEXT_BLUE << (s) << TEXT_NORMAL << std::endl; \
	} while(false)

#define _CYAN(s) do { \
	std::cout << TEXT_CYAN << (s) << TEXT_NORMAL << std::endl; \
	} while(false)

#define _MAGENTA(s) do { \
	std::cout << TEXT_MAGENTA << (s) << TEXT_NORMAL << std::endl; \
	} while(false)

#define _WHITE(s) do { \
	std::cout << TEXT_WHITE << (s) << TEXT_NORMAL << std::endl; \
	} while(false)
#endif  // __cplusplus

#endif  // __TEXTCOLOR_H__
