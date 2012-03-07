/*
 Copyright (C) 1999-2004 IC & S  dbmail@ic-s.nl
 Copyright (c) 2004-2011 NFG Net Facilities Group BV support@nfg.nl

 This program is free software; you can redistribute it and/or 
 modify it under the terms of the GNU General Public License 
 as published by the Free Software Foundation; either 
 version 2 of the License, or (at your option) any later 
 version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
 * dbase driver header file
 * Functions for database communication 
 *
 * should connect to any dbXXX.c file where XXXX is the dbase to be used
 */

#ifndef  _DB_H
#define  _DB_H

#include "dbmail.h"

#define DUMP_BUF_SIZE 1024
#define ICHECK_RESULTSETSIZE 1000000
#define MAX_EMAIL_SIZE 250

/* config types */
#define CONFIG_MANDATORY 1
#define CONFIG_EMPTY 0

#define INIT_QUERY \
       char query[DEF_QUERYSIZE]; \
       memset(query,0,DEF_QUERYSIZE)

#define P ConnectionPool_T
#define S PreparedStatement_T
#define R ResultSet_T
#define C Connection_T
#define U URL_T

/**
 * \brief connect to the database
 * \return 
 *         - -1 on failure
 *         - 0 otherwise
 */
int db_connect(void);

/*
 * make sure we're running against a current database layout 
 */
int db_check_version(void);

/* get a connection from the pool */
C db_con_get(void);

gboolean dm_db_ping(void);
void db_con_close(C c);
void db_con_clear(C c);

/**
 * \brief disconnect from database server
 * \return 0
 */
int db_disconnect(void);

void log_query_time(char *query, struct timeval before, struct timeval after);

S db_stmt_prepare(C c,const char *query, ...);
int db_stmt_set_str(S stmt, int index, const char *x);
int db_stmt_set_int(S stmt, int index, int x);
int db_stmt_set_u64(S stmt, int index, u64_t x);
int db_stmt_set_blob(S stmt, int index, const void *x, int size);
gboolean db_stmt_exec(S stmt);
R db_stmt_query(S stmt);

/**
 * \brief execute a database query
 * \param the_query the SQL query to execute
 * \return 
 *         - 0 on success
 *         - 1 on failure
 */

R db_query(C c, const char *the_query, ...);
gboolean db_exec(C c, const char *the_query, ...);
gboolean db_update(const char *q, ...);

int db_result_next(R r);
/**
 * \brief get number of fields in result set.
 * \return
 *      - 0 on failure or no fields
 *      - number of fields otherwise
 */
unsigned db_num_fields(R r);

/** \defgroup db_result_get_group Variations of db_result_get()
 * \brief get a single result field row from the result set
 * \param row result row to get it from
 * \param field field (=column) to get result from
 * \return
 *     - pointer to string holding result. 
 *     - NULL if no such result
 */
const char *db_result_get(R r, unsigned field);

/** \ingroup db_result_get_group
 * \brief Returns the result as an Integer
 */
int db_result_get_int(R r, unsigned field);

/** \ingroup db_result_get_group
 * \brief Returns the result as an Integer from 0 to 1
 */
int db_result_get_bool(R r, unsigned field);

/** \ingroup db_result_get_group
 * \brief Returns the result as an Unsigned 64 bit Integer
 */
u64_t db_result_get_u64(R r, unsigned field);

const void * db_result_get_blob(R r, unsigned field, int *size);

/**
 * \brief return the ID generated for an AUTO_INCREMENT column
 *        by the previous INSERT query.
 * \return 
 *       - 0 if no such id (if for instance no AUTO_INCREMENT
 *          value was generated).
 *       - the id otherwise
 */
u64_t db_insert_result(C c, R r);

/**
 * \brief return the ID generated by PK TRIGGER for column
 *        by the previous INSERT query.
 * \return 
 *       - 0 if no such id (if for instance no AUTO_INCREMENT
 *          value was generated).
 *       - the id otherwise
 */
u64_t db_get_pk(C c, const char *table);

/**
 * begin transaction
 * \return 
 *     - -1 on error
 *     -  0 otherwise
 */
int db_begin_transaction(C c);

/**
 * commit transaction
 * \return
 *      - -1 on error
 *      -  0 otherwise
 */
int db_commit_transaction(C c);

/**
 * rollback transaction
 * \return 
 *     - -1 on error
 *     -  0 otherwise
 */
int db_rollback_transaction(C c);

/*
 *
 * savepoint setup
 *
 */
// public calls
int db_savepoint(C c, const char * id);
int db_savepoint_rollback(C c, const char *id);

// private calls
int db_savepoint_transaction(C c, const char*);
int db_rollback_savepoint_transaction(C c, const char*);



/*
 * helpers 
 *
 */
int mailbox_is_writable(u64_t mailbox_idnr);

struct mailbox_match {
	char * sensitive;
	char * insensitive;
};

struct mailbox_match * mailbox_match_new(const char *s);
void mailbox_match_free(struct mailbox_match *m);


/*
 * accessors
 *
 */

/**
 * \brief check database for existence of usermap table
 * \return
 * -  0 on table not found
 * -  1 on table found
 */
int db_use_usermap(void);

/**
 * \brief check if username exists in the usermap table
 * \param int tx filehandle of connected client
 * \param userid login to lookup
 * \param real_username contains userid after successful lookup
 * \return
 *  - -1 on db failure
 *  -  0 on success
 *  -  1 on not found
 */
int db_usermap_resolve(clientbase_t *ci, const char *userid, char * real_username);


/**
 * \brief get the physmessage_id from a message_idnr
 * \param message_idnr
 * \param physmessage_id will hold physmessage_id on return. Must hold a valid
 * pointer on call.
 * \return 
 *     - -1 on error
 *     -  0 if a physmessage_id was found
 *     -  1 if no message with this message_idnr found
 * \attention function will fail and halt program if physmessage_id is
 * NULL on call.
 */
int db_get_physmessage_id(u64_t message_idnr, /*@out@*/ u64_t * physmessage_id);


/**
 * \brief return number of bytes used by user identified by userid
 * \param user_idnr id of the user (from users table)
 * \param curmail_size will hold current mailsize used (should hold a valid 
 * pointer on call).
 * \return
 *          - -1 on failure<BR>
 *          -  1 otherwise
 */
int dm_quota_user_set(u64_t user_idnr, u64_t size);
int dm_quota_user_get(u64_t user_idnr, u64_t *size);
int dm_quota_user_dec(u64_t user_idnr, u64_t size);
int dm_quota_user_inc(u64_t user_idnr, u64_t size);

/**
 * \brief finds all users which need to have their curmail_size (amount
 * of space used by user) updated. Then updates this number in the
 * database
 * \return 
 *     - -2 on memory error
 *     - -1 on database error
 *     -  0 on success
 */
int dm_quota_rebuild(void);

/**
 * \brief performs a recalculation of used quotum of a user and puts
 *  this value in the users table.
 * \param user_idnr
 * \return
 *   - -1 on db_failure
 *   -  0 on success
 */
int dm_quota_rebuild_user(u64_t user_idnr);

/**
 * \brief get user idnr of a message. 
 * \param message_idnr idnr of message
 * \return 
 *              - -1 on failure
 * 		- 0 if message is located in a shared mailbox.
 * 		- user_idnr otherwise
 */


/* auto reply and auto notify */
int db_get_notify_address(u64_t user_idnr, char **notify_address);
int db_get_reply_body(u64_t user_idnr, char **reply_body);

u64_t db_get_useridnr(u64_t message_idnr);

/**
 * \brief log IP-address for POP/IMAP_BEFORE_SMTP. If the IP-address
 *        is already logged, it's timestamp is renewed.
 * \param ip the ip (xxx.xxx.xxx.xxx)
 * \return
 *        - -1 on database failure
 *        - 0 on success
 * \bug timestamp!
 */
int db_log_ip(const char *ip);

/**
 * \brief cleanup database tables
 * \return 
 *       - -1 on database failure
 *       - 0 on success
 */
int db_cleanup(void);

/**
 * \brief execute cleanup of database tables (have no idea why there
 *        are two functions for this..)
 * \param tables array of char* holding table names 
 * \param num_tables number of tables in tables
 * \return 
 *    - -1 on db failure
 *    - 0 on success
 */
int db_do_cleanup(const char **tables, int num_tables);

/**
* \brief remove all mailboxes for a user
* \param user_idnr
* \return 
*      - -2 on memory failure
*      - -1 on database failure
*      - 0 on success
*/
int db_empty_mailbox(u64_t user_idnr);

/**
 * \brief fetch the size of a mailbox
 * \param mailbox_idnr
 * \param only_deleted get only the size of all messages flagged for deletion
 * \param mailbox_size pointer to result value
 * \return
 * 	- -1 on database failure
 * 	- 0 on success
 */
int db_get_mailbox_size(u64_t mailbox_idnr, int only_deleted, u64_t * mailbox_size);

/**
* \brief check for all message blocks  that are not connected to
*        a physmessage. This can only occur when in use with a 
*        database that does not check for foreign key constraints.
* \param lost_list pointer to a list which will contain all lost blocks.
*        this list needs to be empty on call to this function.
* \return 
*      - -2 on memory error
*      - -1 on database error
*      - 0 on success
* \attention caller should free this memory
*/

int db_icheck_partlists(gboolean cleanup);
int db_icheck_mimeparts(gboolean cleanup);
int db_icheck_physmessages(gboolean cleanup);

/** 
 * \brief check for cached header values
 *
 */
int db_icheck_headercache(GList **lost);
int db_set_headercache(GList *lost);

/**
 * \brief check for rfcsize in physmessage table
 *
 */
int db_icheck_rfcsize(GList **lost);
int db_update_rfcsize(GList *lost);

/**
 * \brief check for cached envelopes
 *
 */

int db_icheck_envelope(GList **lost);
int db_set_envelope(GList *lost);

/**
 * \brief set status of a message
 * \param message_idnr
 * \param status new status of message
 * \return result of Connection_executeQuery()
 */
int db_set_message_status(u64_t message_idnr, MessageStatus_t status);

/**
 * \brief delete a message 
 * \param message_idnr
 * \return 
 *     - -1 on error
 *     -  1 on success
 */
int db_delete_message(u64_t message_idnr);

/**
 * \brief delete a mailbox. 
 * \param mailbox_idnr
 * \param only_empty if non-zero the mailbox will only be emptied,
 *        i.e. all messages in it will be deleted.
 * \param update_curmail_size if non-zero the curmail_size of the
 *        user will be updated.
* \return 
*    - -1 on database failure
*    - 0 on success
* \attention this function is unable to delete shared mailboxes
*/
int db_delete_mailbox(u64_t mailbox_idnr, int only_empty,
		      int update_curmail_size);

char * db_get_message_lines(u64_t message_idnr, long lines);

/**
 * \brief update POP3 session
 * \param session_ptr pointer to POP3 session
 * \return
 *     - -1 on failure
 *     - 0 on success
 * \attention does not update shared mailboxes (which should
 * not be nessecary, because the shared mailboxes are not 
 * touched by POP3
 */
int db_update_pop(ClientSession_t * session_ptr);

/* mailbox functionality */
/** 
 * \brief find mailbox "name" for a user
 * \param name name of mailbox
 * \param user_idnr
 * \param mailbox_idnr will hold mailbox_idnr after return. Must hold a valid
 * pointer on call.
 * \return
 *      - -1 on failure
 *      - 0 if mailbox not found
 *      - 1 if found
 */
int db_findmailbox(const char *name, u64_t user_idnr,
		   /*@out@*/ u64_t * mailbox_idnr);
/**
 * \brief finds all the mailboxes owned by owner_idnr who match 
 *        the pattern.
 * \param owner_idnr
 * \param pattern pattern
 * \param children pointer to a list of mailboxes conforming to
 *        pattern. This will be filled when the function
 *        returns and needs to be free-d by caller
 * \param only_subscribed only search in subscribed mailboxes.
 * \return 
 *      - -1 on failure
 *      - 0 on success
 */
int db_findmailbox_by_regex(u64_t owner_idnr, const char *pattern, GList ** children, int only_subscribed);
/**
 * \brief find owner of a mailbox
 * \param mboxid id of mailbox
 * \param owner_id will hold owner of mailbox after return
 * \return
 *    - -1 on db error
 *    -  0 if owner not found
 *    -  1 if owner found
 */
int db_get_mailbox_owner(u64_t mboxid, /*@out@*/ u64_t * owner_id);

/**
 * \brief check if a user is owner of the specified mailbox 
 * \param userid id of user
 * \param mboxid id of mailbox
 * \return
 *     - -1 on db error
 *     -  0 if not user
 *     -  1 if user
 */
int db_user_is_mailbox_owner(u64_t userid, u64_t mboxid);

/**
 * \brief create a new mailbox
 * \param name name of mailbox
 * \param owner_idnr owner of mailbox
 * \return 
 *    - -1 on failure
 *    -  0 on success
 */
int db_createmailbox(const char *name, u64_t owner_idnr,
		     u64_t * mailbox_idnr);
/**
 * \brief Create a mailbox, recursively creating its parents.
 * \param mailbox Name of the mailbox to create
 * \param source Upstream source of this mailbox spec
 * \param owner_idnr Owner of the mailbox
 * \param mailbox_idnr Fills the pointer with the mailbox id
 * \param message Fills the pointer with a static pointer to a message
 * \return
 *    -  0 on success
 *    -  1 on failure
 *    - -1 on error
 */
int db_mailbox_create_with_parents(const char * mailbox, mailbox_source_t source,
		u64_t owner_idnr, u64_t * mailbox_idnr, const char * * message);

/**
 * \brief Splits a mailbox name into pieces on '/'
 * \param mailbox Name of the mailbox to split
 * \param owner_idnr Owner of the mailbox
 * \param mailboxes Filled with a list of mailbox piece names
 * \param errmsg Fills the pointer with a static pointer to a message
 * \return
 *    -  0 on success
 *    -  1 on failure
 *    - -1 on error
 */
GList * db_imap_split_mailbox(const char *mailbox, u64_t owner_idnr, const char ** errmsg);

/**
 * \brief set permission on a mailbox (readonly/readwrite)
 * \param mailbox_id idnr of mailbox
 * \return
 *  - -1 on database failure
 *  - 0 on success
 *  - 1 on failure
 */
int db_mailbox_set_permission(u64_t mailbox_id, int permission);

/**
 * \brief find a mailbox, create if not found
 * \param name name of mailbox
 * \param owner_idnr owner of mailbox
 * \return 
 *    - -1 on failure
 *    -  0 on success
 */
int db_find_create_mailbox(const char *name, mailbox_source_t source,
		u64_t owner_idnr, /*@out@*/ u64_t * mailbox_idnr);
/**
 * \brief produce a list containing the UID's of the specified
 *        mailbox' children matching the search criterion
 * \param mailbox_idnr id of parent mailbox
 * \param user_idnr
 * \param children will hold list of children
 * \param filter search filter
 * \return 
 *    - -1 on failure
 *    -  0 on success
 */
int db_listmailboxchildren(u64_t mailbox_idnr, u64_t user_idnr, GList ** children);

/**
 * \brief check if mailbox is selectable
 * \param mailbox_idnr
 * \return 
 *    - -1 on failure
 *    - 0 if not selectable
 *    - 1 if selectable
 */
int db_isselectable(u64_t mailbox_idnr);
/**
 * \brief check if mailbox has no_inferiors flag set.
 * \param mailbox_idnr
 * \return
 *    - -1 on failure
 *    - 0 flag not set
 *    - 1 flag set
 */
int db_noinferiors(u64_t mailbox_idnr);
/** 
 * \brief append message to mailbox
 * \param msgdata raw message data
 * \param mailbox_idnr destination mailbox
 * \param user_idnr who is appending
 * \param internal_date optional
 * \param msg_idnr result
 * \return 
 * 		- -1 on failure
 * 		- 0 on success
 */

int db_append_msg(const char *msgdata, u64_t mailbox_idnr, u64_t user_idnr, timestring_t internal_date, u64_t * msg_idnr);

/**
 * \brief move all messages from one mailbox to another.
 * \param mailbox_to idnr of mailbox to move messages from.
 * \param mailbox_from idnr of mailbox to move messages to.
 * \return 
 * 		- -1 on failure
 * 		- 0 on success
 */
int db_movemsg(u64_t mailbox_to, u64_t mailbox_from);
/**
 * \brief copy a message to a mailbox
 * \param msg_idnr
 * \param mailbox_to mailbox to copy to
 * \param user_idnr user to copy the messages for.
 * \return 
 * 		- -2 if the quotum is exceeded
 * 		- -1 on failure
 * 		- 0 on success
 */
int db_copymsg(u64_t msg_idnr, u64_t mailbox_to,
	       u64_t user_idnr, u64_t * newmsg_idnr);

/**
 * \brief check if mailbox already holds message with message-id
 * \param mailbox_idnr
 * \param messageid Message-ID header to check for
 * \return number of matching messages or -1 on failure
 */
int db_mailbox_has_message_id(u64_t mailbox_idnr, const char *messageid);

/**
 * \brief get name of mailbox
 * \param mailbox_idnr
 * \param name will hold the name
 * \return 
 * 		- -1 on failure
 * 		- 0 on success
 * \attention name should be large enough to hold the name 
 * (preferably of size IMAP_MAX_MAILBOX_NAMELEN + 1)
 */


int db_getmailboxname(u64_t mailbox_idnr, u64_t user_idnr, char *name);
/**
 * \brief set name of mailbox
 * \param mailbox_idnr
 * \param name new name of mailbox
 * \return 
 * 		- -1 on failure
 * 		- 0 on success
 */
int db_setmailboxname(u64_t mailbox_idnr, const char *name);

/**
 * \brief subscribe to a mailbox.
 * \param mailbox_idnr mailbox to subscribe to
 * \param user_idnr user to subscribe
 * \return 
 * 		- -1 on failure
 * 		- 0 on success
 */
int db_subscribe(u64_t mailbox_idnr, u64_t user_idnr);
/**
 * \brief unsubscribe from a mailbox
 * \param mailbox_idnr
 * \param user_idnr
 * \return
 * 		- -1 on failure
 * 		- 0 on success
 */
int db_unsubscribe(u64_t mailbox_idnr, u64_t user_idnr);

/* message functionality */

/**
 * \brief get a flag for a message specified by flag_name.
 * \param flag_name name of flag
 * \param msg_idnr
 * \return 
 * 		- -1 on failure
 * 		- 0 if flag is not set or a non-existent flag is asked.
 * 		- 1 if flag is set.
 */
int db_get_msgflag(const char *flag_name, u64_t msg_idnr);

/**
 * \brief set flags for a message
 * \param msg_idnr
 * \param flags Array of IMAP_NFLAGS elements. See
 *               imapcommand.c for details.
 * \param action_type 
 *        - IMAPFA_REPLACE new set will be exactly as flags,
 *          with '1' for set, and '0' for not set.
 *        - IMAPFA_ADD set all flags which have a '1' as value
 *          in flags
 *        - IMAPFA_REMOVE clear all flags that have '1' as value
 *          in flags
 * \return 
 * 		- -1 on failure
 * 		-  0 on success
 */
int db_set_msgflag(u64_t msg_idnr, int *flags, GList *keywords, int action_type, MessageInfo *msginfo);

/**
 * \brief set one right in an acl for a user
 * \param userid id of user
 * \param mboxid id of mailbox
 * \param right_flag string holding the acl to set
 * \param set 0 if flag will be set to 0, 1 otherwise
 * \return 
 *     - -1 on error
 *     -  1 on success
 * \note if the user has no acl for this mailbox, it
 *       will be created.
 */
int db_acl_set_right(u64_t userid, u64_t mboxid, const char *right_flag, int set);

/**
 * \brief delete an ACL for a user, mailbox
 * \param userid id of user
 * \param mboxid id of mailbox
 * \return
 *      - -1 on db failure
 *      -  1 on success
 */
int db_acl_delete_acl(u64_t userid, u64_t mboxid);

/**
 * \brief get a list of all identifiers which have rights on a mailbox.
 * \param mboxid id of mailbox
 * \param identifier_list list of identifiers
 * \return 
 *     - -2 on mem failure
 *     - -1 on db failure
 *     -  1 on success
 * \note identifier_list needs to be empty on call.
 */
int db_acl_get_identifier(u64_t mboxid, /*@out@*/ GList  **identifier_list);
/**
 * constructs a string for use in queries. This is used to not be dependent
 * on the internal representation of a date in the database. Whenever the
 * date is queried for in a query, this function is used to get the right
 * database function for the query (TO_CHAR(date,format) for Postgres, 
 * DATE_FORMAT(date, format) for MySQL).
 */
int date2char_str(const char *column, field_t *frag);
int char2date_str(const char *date, field_t *frag);


/* 
 * db-user accessors
 */

int db_user_exists(const char *username, u64_t * user_idnr);
int db_user_create_shadow(const char *username, u64_t * user_idnr);
int db_user_create(const char *username, const char *password, const char *enctype,
		 u64_t clientid, u64_t maxmail, u64_t * user_idnr); 
int db_user_find_create(u64_t user_idnr);
int db_user_delete(const char * username);
int db_user_rename(u64_t user_idnr, const char *new_name); 
int db_user_log_login(u64_t user_idnr);

int db_change_mailboxsize(u64_t user_idnr, u64_t new_size);

/* auto-reply cache */
int db_replycache_register(const char *to, const char *from, const char *handle);
int db_replycache_validate(const char *to, const char *from, const char *handle, int days);
int db_replycache_unregister(const char *to, const char *from, const char *handle);

/* get driver specific SQL snippets */
const char * db_get_sql(sql_fragment_t frag);
char * db_returning(const char *s);

int db_mailbox_seq_update(u64_t mailbox_id);

int db_rehash_store(void);

#endif
