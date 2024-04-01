/* vkoem.h */
/* This file should match VKOEM.INC for the keyboard driver */

/*
** 26 jun 87	plb	Made Olivetti virtual codes corresp. to VKOEM.INC
*/

/* This group is fairly standard */
#define VK_OEM_NUMBER	0x090		/* NumLock */
#define VK_OEM_SCROLL	0x091		/* ScrollLock */
#define VK_OEM_1	0x0BA		/* ';:' for US */
#define VK_OEM_PLUS	0x0BB		/* '+' any country */
#define VK_OEM_COMMA	0x0BC		/* ',' any country */
#define VK_OEM_MINUS	0x0BD		/* '-' any country */
#define VK_OEM_PERIOD	0x0BE		/* '.' any country */
#define VK_OEM_2	0x0BF		/* '/?' for US */
#define VK_OEM_3	0x0C0		/* '`~' for US */
#define VK_OEM_4	0x0DB		/* '[{' for US */
#define VK_OEM_5	0x0DC		/* '\|' for US */
#define VK_OEM_6	0x0DD		/* ']}' for US */
#define VK_OEM_7	0x0DE		/* ''"' for US */

/* additional for Olivetti */
#define VK_OEM_8	0x0DF

#define VK_F17		0x0e0		/* F17 key */
#define VK_F18		0x0e1		/* F18 key */
#define VK_OEM_102	0x0e2		/* <> or /| key on 101/102 keyboard */
#define VK_ICO_HELP	0x0E3
#define VK_ICO_00	0x0E4
#define VK_ICO_CLEAR	0x0E6

#define VK_OEM_ALT	0x092	/* not required in 2.00 driver */
