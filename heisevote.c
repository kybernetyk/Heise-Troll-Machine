/*

  ___ ______________.___  ____________________
 /   |   \_   _____/|   |/   _____/\_   _____/
/    ~    \    __)_ |   |\_____  \  |    __)_ 
\    Y    /        \|   |/        \ |        \
 \___|_  /_______  /|___/_______  //_______  /
       \/        \/             \/         \/ 

___________              .__   .__   
\__    ___/_______  ____ |  |  |  |  
  |    |   \_  __ \/  _ \|  |  |  |  
  |    |    |  | \(  <_> )  |__|  |__
  |____|    |__|   \____/|____/|____/
                                     

   _____                 .__    .__               
  /     \ _____     ____ |  |__ |__| ____   ____  
 /  \ /  \\__  \  _/ ___\|  |  \|  |/    \_/ __ \ 
/    Y    \/ __ \_\  \___|   Y  \  |   |  \  ___/ 
\____|__  (____  / \___  >___|  /__|___|  /\___  >
        \/     \/      \/     \/        \/     \/


 Heise Mass Voting - just like real democracy <3
 
 ]: needs libcurl :[
 compile with:
	 clang -lcurl -o heisevote heisevote.c
*/

#include <stdio.h>
#include <curl/curl.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

int g_debug = 1;

typedef struct _hp_curl_session
{
	void *buffer;
	size_t bytes_read;
} hp_curl_session;

typedef struct _hp_credentials
{
	char *account;
	char *password;
} hp_credentials;


typedef struct _hp_vote
{
	char *post_url;
	int vote_value;
} hp_vote;

typedef struct _hp_credential_db_handle
{
	FILE *f_db;
} hp_credential_db_handle;

void usage (const char *arg0)
{
	printf("usage: %s <arguments>\n", arg0);
	printf("\t-f | --credfile\tfile in the format of login:password [requried]\n");
	printf("\t-p | --posturl\turl of post to vote up/down [required]\n");
	printf("\t-v | --vote <-2 or 2>\tvalue of vote [required]\n");
	printf("\t-h | --help recursion mode [mandatory]\n");
	printf("example: %s -f heise_db.txt -p http://www.heise[...]/read/ -v '-2'\n",arg0);
}

size_t my_curl_callback (void *buffer, size_t size, size_t nmemb, void *in_session)
{
	hp_curl_session *session = (hp_curl_session *)in_session;
	
	if (session->buffer == NULL)
	{
		session->buffer = malloc(0x40000); //256kb malloc(csize*nmemb);
	}
/*	else
	{
		session->buffer = realloc(session->buffer, session->bytes_read + (size*nmemb));
	}*/
	
	memcpy(session->buffer+session->bytes_read, buffer, size*nmemb);
	session->bytes_read += (size * nmemb);
	
	
	return size * nmemb;
}

int login (CURL *curl_handle, hp_credentials *credentials)
{
	printf("performing login with credentials %s:%s ...\n", credentials->account, credentials->password);
	
	hp_curl_session session;
	session.buffer = NULL;
	session.bytes_read = 0;
	
	char postfields[512];
	memset(postfields,0x00,512);
	sprintf(postfields,"username=%s&password=%s&permanent=ON&lock_ip=ON&rm=do_login&dirid=1&forward=/&skin=default&submit=Einloggen",credentials->account,credentials->password);
	
//	printf("postfields: %s\n",postfields);
	
	//setup curl for http get
//	curl_easy_setopt(curl_handle, CURLOPT_HTTPGET,1);
	
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, postfields);
	curl_easy_setopt(curl_handle, CURLOPT_URL, "http://www.heise.de/userdb/sso");
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE,"/tmp/hfcookie");

	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, my_curl_callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &session);

	
	CURLcode res = curl_easy_perform(curl_handle);
	if (res != CURLE_OK)
	{
		fprintf(stderr,"\tcurl returned NOT OK!\n");
		free(session.buffer);
		return 0;
	}
	
	
	if (strcasestr(session.buffer, "Der User existiert nicht oder das Passwort ist falsch.") != NULL)
	{
		fprintf(stderr,"\tlogin error: wrong username/password\n");
		free(session.buffer);
		return 0;
	}

	printf("\tlogin ok\n");
	printf("ok\n");
	free(session.buffer);
	return 1;
}


size_t retrieve_page (CURL *curl_handle, hp_curl_session *session, char *url)
{
	curl_easy_setopt(curl_handle, CURLOPT_HTTPGET,1);
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
	//	curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE,"/tmp/hfcookie");
	
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, my_curl_callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, session);
	
	
	CURLcode res = curl_easy_perform(curl_handle);
	if (res != CURLE_OK)
	{
		fprintf(stderr,"\tcurl returned NOT OK!\n");
		//free(session.buffer);
		return 0;
	}
	
	
	
	return session->bytes_read;
}

int logout (CURL *curl_handle)
{
	hp_curl_session session;
	session.buffer = NULL;
	session.bytes_read = 0;

	if (!retrieve_page (curl_handle, &session, "http://www.heise.de/userdb/sso?rm=do_logout&dirid=1"))
	{
		fprintf(stderr,"\tcould not retrieve logout page :[\n");
		free(session.buffer);
		return 0;
	}
	free(session.buffer);	
	return 1;
}




int dumpuser (CURL *curl_handle)
{
	hp_curl_session session;
	session.buffer = NULL;
	session.bytes_read = 0;
	
	if (!retrieve_page (curl_handle, &session, "http://www.heise.de/userdb/register?rm=show_userdata&dirid=1&skin=default"))
	{
		fprintf(stderr,"\tcould not retrieve dump page :[\n");
		free(session.buffer);
		return 0;
	}
	
	printf("dump: %s\n",(char *)session.buffer);
	
	free(session.buffer);
		   
	return 1;
}

int vote_post (CURL *curl_handle, hp_vote *vote)
{
	printf("retrieving post from %s ...\n", vote->post_url);
	
	hp_curl_session session;
	session.buffer = NULL;
	session.bytes_read = 0;
	
	if (!retrieve_page(curl_handle, &session, vote->post_url))
	{
		fprintf(stderr,"\tcould not retrieve post %s\n",vote->post_url);
		free(session.buffer);
		return 0;
	}
	printf("\tgot post (size %li byte) ...\n",session.bytes_read);	

	// this won't show always correctly as this session info is not saved that long on heise forums
	char *p_current_standing = strcasestr(session.buffer,"vote_posting");
	if (!p_current_standing)
	{	
		p_current_standing = "No standing yet.";
		
	}
	else
	{
		p_current_standing = strcasestr(p_current_standing, "alt=\"");
		p_current_standing += strlen("alt=\"");
		
		char *tmp = strcasestr(p_current_standing, "\"");
		char oldval = *tmp;
		*tmp = 0;

		char *tmp2 = alloca(255);
		memset(tmp2,0x00,255);
		sprintf(tmp2,"%s",p_current_standing);
		*tmp = oldval;
		p_current_standing = tmp2;
	}
	printf("\tcurrent standing: %s\n",p_current_standing);
	
	// this won't trigger correctly too after some time :/
	char *p_already_voted = strcasestr(session.buffer, "Beitrag wurde bereits bewertet");
	if (p_already_voted)
	{
		fprintf(stderr, "\talready voted on this post!\n");
		free(session.buffer);
		return 1;
	}
	
	//start of the votelink block containing our needed MD5
	char *p_votelinks_start = strcasestr(session.buffer, "<div class=\"tovote_links\">Beitrag bewerten:");
	if (!p_votelinks_start)
	{
		fprintf(stderr, "\tcould not locate votelinks start position ...\n");
		free(session.buffer);
		return 0;
	}
	
	//end of the votelink block
	char *p_votelinks_end = strcasestr(p_votelinks_start, "</span> <p></p></div>");
	if (!p_votelinks_end)
	{
		fprintf(stderr, "\tcould not locate votelinks end position ...\n");
		free(session.buffer);
		return 0;
	}
	*p_votelinks_end = 0;
	
	//extract the MD5
	char *p_md5_start = strcasestr(p_votelinks_start, "MD5-");
	if (!p_md5_start)
	{
		fprintf(stderr, "\tcould not locate md5 start ...\n");
		free(session.buffer);
		return 0;
	}
	
	char *p_md5_end = strcasestr(p_md5_start, "/");
	if (!p_md5_end)
	{
		fprintf(stderr, "\tcould not locate md5 end ...\n");
		free(session.buffer);
		return 0;
	}
	*p_md5_end = 0;

	//now get forum and post id from user supplied url
//	http://www.heise.de/open/news/foren/S-Re-Ja-wieee-noch-keine-M-Fanboys-hier/forum-184075/msg-18969944/read/
	char *tmp_vote_url = alloca(strlen(vote->post_url)+1);
	memset(tmp_vote_url, 0x00, strlen(vote->post_url)+1);
	memcpy(tmp_vote_url, vote->post_url, strlen(vote->post_url));
	
	char *p_forum_id_start = strcasestr(tmp_vote_url, "/forum-");
	if (!p_forum_id_start)
	{
		fprintf(stderr, "\tcould not get forum id start ...\n");
		free(session.buffer);
		return 0;
	}
	p_forum_id_start += 1;
	char *p_forum_id_end = strcasestr(p_forum_id_start, "/");
	if (!p_forum_id_end)
	{
		fprintf(stderr, "\tcould not get forum id end ...\n");
		free(session.buffer);
		return 0;
	}

	char *p_msg_id_start = strcasestr(tmp_vote_url, "/msg-");
	if (!p_msg_id_start)
	{
		fprintf(stderr, "\tcould not get msg id start ...\n");
		free(session.buffer);
		return 0;
	}
	p_msg_id_start += 1;
	char *p_msg_id_end = strcasestr(p_msg_id_start, "/");
	if (!p_msg_id_end)
	{
		fprintf(stderr, "\tcould not get msg id end ...\n");
		free(session.buffer);
		return 0;
	}
		
	//terminate
	*p_forum_id_end = 0;
	*p_msg_id_end = 0;
	
	//the vote values
	char *p_vote_value = 0;
	switch (vote->vote_value) 
	{
		case -2:
			p_vote_value = "S-voellig-belangloser-Beitrag";
			break;
		case 2:
			p_vote_value = "S-unbedingt-lesenswert";
		default:
			break;
	}
	if (p_vote_value == 0)
	{
		fprintf(stderr, "\tvote value of %i not supported!\n",vote->vote_value);
		free(session.buffer);
		return 0;
	}
	
	char *p_vote_postfix = alloca(512);
	memset(p_vote_postfix, 0x00, 512);
	sprintf(p_vote_postfix,"postvote-%i",vote->vote_value);
	
	char *p_vote_prefix_start = tmp_vote_url;
	
	char *p_vote_prefix_end = strstr(tmp_vote_url, "/S-");
	if (!p_vote_prefix_end)
	{
		fprintf(stderr, "\tcould not terminate prefix!\n");
		free(session.buffer);
		return 0;
	}
	*p_vote_prefix_end = 0;
	
	
	char *p_final_vote_url = alloca(1024);
	memset(p_final_vote_url, 0x00, 1024);

	sprintf(p_final_vote_url, "%s/%s/%s/%s/read/%s/%s/",
			p_vote_prefix_start,
			p_vote_value,
			p_forum_id_start,
			p_msg_id_start,
			p_md5_start,
			p_vote_postfix);

	if (g_debug)
	{	
		printf("\textracted infos:\n\t\tmd5: %s\n\t\tmsg: %s\n\t\tforum: %s\n\t\tvoteval: %s\n\t\tpostfix: %s\n\t\tprefix: %s\n",
			   p_md5_start, 
			   p_msg_id_start, 
			   p_forum_id_start, 
			   p_vote_value,
			   p_vote_postfix,
			   p_vote_prefix_start);
		
		printf("\tfinal url: %s\n",p_final_vote_url);
	}
	
	free(session.buffer);
	
	
	//////////////////////////////////////
	//now let's do voting
	session.buffer = NULL;
	session.bytes_read = 0;
	
	
	if (!retrieve_page (curl_handle, &session, p_final_vote_url))
	{
		fprintf(stderr,"\tcould not retrieve voting page %s\n",p_final_vote_url);
		free(session.buffer);
		return 0;
	}
	
	printf("\tvote sent ...\n");	
	printf("\tgot confirmation page (size %li byte) ...\n",session.bytes_read);	

	p_already_voted = strcasestr(session.buffer, "Beitrag wurde bereits bewertet");
	if (p_already_voted)
	{
		printf("\tlol, you already voted on this post, dummy!\n");
	}
	
	
	p_current_standing = strcasestr(session.buffer,"vote_posting");
	if (!p_current_standing)
	{	
		p_current_standing = "No standing yet.";
		
	}
	else
	{
		p_current_standing = strcasestr(p_current_standing, "alt=\"");
		p_current_standing += strlen("alt=\"");
		
		char *tmp = strcasestr(p_current_standing, "\"");
		char oldval = *tmp;
		*tmp = 0;
		
		char *tmp2 = alloca(255);
		memset(tmp2,0x00,255);
		sprintf(tmp2,"%s",p_current_standing);
		*tmp = oldval;
		p_current_standing = tmp2;
	}
	printf("\tnew standing: %s\n",p_current_standing);
	printf("ok\n");
	
	free(session.buffer);
	return 1;
	
}

int open_db_file (char *filename, hp_credential_db_handle *db_handle)
{
	db_handle->f_db = fopen(filename, "r");
	if (!db_handle->f_db)
	{
		fprintf(stderr, "could not open db file %s\n", filename);
		return 0;
	}
		
	return 1;
}

int get_next_login (hp_credential_db_handle *db_handle, hp_credentials *p_out_credential)
{
	char *line = malloc(512);
	if (!fgets(line, 512, db_handle->f_db))
	{
		free(line);
		return 0;
	}
	
	char *p_colon = strstr(line, ":");
	if (!p_colon)
	{
		free(line);
		return 0;
	}
	char *p_newline = strstr(line, "\n");
	if (p_newline)
		*p_newline = 0;
		
	char *p_password = p_colon+1;
	char *p_login = line;
	*p_colon = 0;

	char *c_login = malloc(strlen(p_login)+1);
	memset(c_login,0x00,strlen(p_login)+1);
	memcpy(c_login, p_login,strlen(p_login));
	
	
	char *c_password = malloc(strlen(p_password)+1);
	memset(c_password,0x00,strlen(p_password)+1);
	memcpy(c_password, p_password,strlen(p_password));
	
	p_out_credential->account = c_login;
	p_out_credential->password = c_password;
	
	return 1;
}

int close_db_file (hp_credential_db_handle *db_handle)
{
	fclose(db_handle->f_db);
	db_handle->f_db = 0;
	
	return 1;
}

void free_credential (hp_credentials *credential)
{
	free(credential->account), credential->account = 0;
	free(credential->password), credential->password = 0;;
}

int main (int argc, const char *argv[]) 
{
	int option_index = 0;

	char *db_file = 0;
	char *post_url = 0;
	int vote_val = 0;
	
	if (argc == 1)
	{
		usage(argv[0]);
		return 1;
	}
	
	while (1)
	{
		static struct option long_options[] =
		{
			{"credfile",     required_argument,       0, 'f'},
			{"posturl",  required_argument,       0, 'p'},
			{"vote",    required_argument, 0, 'v'},
			{"help",    no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};
		
		char c = getopt_long (argc, (char *const *)argv, "f:p:v:h?", long_options, &option_index);
	
		/* Detect the end of the options. */
		if (c == -1)
			break;
		
		switch (c)
		{

			case 'f':
				db_file = malloc(strlen(optarg)+1);
				sprintf(db_file, "%s", optarg);
				break;
				
			case 'p':
				post_url = malloc(strlen(optarg)+1);
				sprintf(post_url, "%s",optarg);
				break;
				
			case 'v':
				vote_val = atoi(optarg);
				break;
				
			case '?':
			case 'h':
				usage (argv[0]);
				break;
				
			default:
				abort ();
		}
	}

	int dienow = 0;
	if (!db_file)
	{
		fprintf(stderr, "specify a credential file with the --credfile argument!\n");
		dienow = 2;
	}
	
	if (!post_url)
	{
		fprintf(stderr, "specify a post url with the --posturl argument!\n");
		dienow = 3;
	}
	
	if (vote_val != -2 && vote_val != 2)
	{
		fprintf(stderr, "specify a vote value of -2 or 2 with the --vote argument!\n");
		dienow = 4;
	}

	if (dienow)
		return dienow;
	
	
	hp_vote vote;
	vote.vote_value = vote_val;
	vote.post_url = post_url;
	
	
	hp_credential_db_handle db_handle;
	if (!open_db_file(db_file, &db_handle))
	{
		return 1;
	}

	
	CURL *curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE,"/dev/null");

	
	hp_credentials credential;
	while (get_next_login(&db_handle, &credential))
	{
		//printf("got credential: l: %s \t\tp: %s\n",credential.account, credential.password);

		if (!login(curl_handle, &credential))
		{
			
			fprintf(stderr,"login error. skipping to next account ...\n\n");
			continue;
		}
		//sleep(7);
		if (!vote_post(curl_handle, &vote))
		{
			//return 2;
			fprintf(stderr,"voting error. skipping to next account ...\n\n");
			continue;
		}
		//sleep(7);	
		logout(curl_handle);
		printf("\n");
		printf("\n");
		
		free_credential (&credential);
	}
	
	close_db_file(&db_handle);
	curl_easy_cleanup(curl_handle);

	free (db_file);
	free (post_url);
	
    return 0;
}
