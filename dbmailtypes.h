/* 
 * dbmailtypes.h
 *
 * a set of data type definitions used at various 
 * places within the dbmail package
 */

#ifndef _DBMAILTYPES_H
#define _DBMAILTYPES_H

#include "memblock.h"
#include "list.h"

#define MAX_SEARCH_LEN 1024


#define MIME_FIELD_MAX 128
#define MIME_VALUE_MAX 4096

/*
 * define some IMAP symbols
 */
enum IMAP4_CLIENT_STATES { IMAPCS_INITIAL_CONNECT, IMAPCS_NON_AUTHENTICATED, 
			   IMAPCS_AUTHENTICATED, IMAPCS_SELECTED, IMAPCS_LOGOUT };

enum IMAP4_FLAGS { IMAPFLAG_SEEN = 0x01, IMAPFLAG_ANSWERED = 0x02, 
		   IMAPFLAG_DELETED = 0x04, IMAPFLAG_FLAGGED = 0x08,
		   IMAPFLAG_DRAFT = 0x10, IMAPFLAG_RECENT = 0x20 };

enum IMAP4_PERMISSION { IMAPPERM_READ = 0x01, IMAPPERM_READWRITE = 0x02 };

enum IMAP4_FLAG_ACTIONS { IMAPFA_NONE, IMAPFA_REPLACE, IMAPFA_ADD, IMAPFA_REMOVE };

enum BODY_FETCH_ITEM_TYPES { BFIT_TEXT, BFIT_HEADER, BFIT_MIME,
			     BFIT_HEADER_FIELDS,
			     BFIT_HEADER_FIELDS_NOT, BFIT_TEXT_SILENT };

/* maximum size of a mailbox name */
#define IMAP_MAX_MAILBOX_NAMELEN 100

/* length of internaldate string */
#define IMAP_INTERNALDATE_LEN 30

/* max length of number/dots part specifier */
#define IMAP_MAX_PARTSPEC_LEN 100



/* use 64-bit unsigned integers as common data type */
typedef unsigned long long u64_t;

/* 
 * (imap) mailbox data type
 */
typedef struct 
{
  u64_t uid,msguidnext;
  unsigned exists,recent,unseen;
  unsigned flags;
  int permission;
  u64_t *seq_list;
} mailbox_t;


/*
 * search data types
 */

enum IMAP_SEARCH_TYPES { IST_SET, IST_SET_UID, IST_FLAG, IST_HDR, IST_HDRDATE_BEFORE,
			 IST_HDRDATE_ON, IST_HDRDATE_SINCE,
			 IST_IDATE, IST_DATA_BODY, IST_DATA_TEXT, IST_SIZE_LARGER, IST_SIZE_SMALLER, 
			 IST_SUBSEARCH_AND, IST_SUBSEARCH_OR, IST_SUBSEARCH_NOT };

typedef struct 
{
  int type;
  u64_t size;
  char search[MAX_SEARCH_LEN];
  char hdrfld[MIME_FIELD_MAX];
  struct list sub_search;
} search_key_t;



/*
 * remembering database positions for mail
 * interpretation of the fields may be db-dependent, see dbmsgbufXXXX.c for details
 * (where XXXX stands for the dbase type)
 */

typedef struct 
{
  u64_t block,pos;
} db_pos_t;


/*
 * RFC822/MIME message data type
 */
typedef struct 
{
  struct list mimeheader;           /* the MIME header of this part (if present) */
  struct list rfcheader;            /* RFC822 header of this part (if present) */
  int message_has_errors;           /* if set the content-type is meaningless */
  db_pos_t bodystart,bodyend;       /* the body of this part */
  u64_t bodysize;
  u64_t bodylines;
  u64_t rfcheadersize;         
  struct list children;             /* the children (multipart msg) */
  u64_t rfcheaderlines;
  u64_t mimerfclines;          
    /* the total number of lines (only specified in case of a MIME msg containing an RFC822 msg) */
} mime_message_t;

 

/* 
 * simple cache mechanism
 */
typedef struct
{
  mime_message_t msg;
  MEM *memdump,*tmpdump;
  u64_t num;
  int file_dumped,msg_parsed;
  u64_t dumpsize;
} cache_t;


#endif

