#!/usr/bin/env python

#   #   #####   #   ####    ######
#   #   #       #   #       #
#####   ####    #   ####    ####   TROLLmachine
#   #   #       #      ##   # 
#   #   #####   #   ####    ######

#
# creates shiny heise accs for trolling lol and puts username:password into heise_db.txt
#
#  no there is no error checking lol.
#

#############################################################################
import sys
import getopt
import httplib, urllib, random

argv = sys.argv

def usage():
	print "omg penis u need ALL teh agruments:"
	print "%s" %(argv[0])
	print "\t-l | --login <account name - that's the account login!>"
	print "\t-p | --password <la password>"
	print "\t-m | --email  <your email address>"
	print "\t-n | --nick <your forum nickname ... yeah confusing>"
	print "\t-g | --givenname <ur given name>"
	print "\t-s | --surname <u're surname>"


num_of_args_needed = 6
nickname = ""						#your account name you will use to log in
givenname = ""								#your troll's given name
name = ""								#your surname
pseudonym = ""				#your nickname that will be visible in the forums
email = ""						#your mail address. (don't use throw aways - their'er ban'd)
password = ""							#lol


##############################################################################
#arg checking
if ((len(argv) != num_of_args_needed+1) and (len(argv) != num_of_args_needed*2+1)):
	usage()
	sys.exit(2)

opts, args = getopt.getopt(argv[1:], "l:p:m:n:g:s:", ['login=','password=','email=','nick=','givenname=','surname='])

for opt, arg in opts:
	if opt in ("-l","--login"):
		nickname = arg
	elif opt in ("-p","--password"):
		password = arg
	elif opt in ("-m","--email"):
		email = arg
	elif opt in ("-n","--nick"):
		pseudonym = arg
	elif opt in ("-g","--givenname"):
		givenname = arg
	elif opt in ("-s","--surname"):
		name = arg

if (len(nickname) <= 0 or len(password) <= 0 or len(email) <= 0 or len(pseudonym) <= 0 or len(givenname) <= 0 or len(name) <= 0):
	usage()
	sys.exit(3)
	
###############################################################################
#phase 1
param_dict = {	'nickname' : "%s" %(nickname), \
								'givenname' : "%s" %(givenname),	\
								'name' : "%s" %(name), \
								'pseudonym' : "%s" %(pseudonym), \
							#	'showrealname' : "1", \
								'email' : "%s" %(email), \
							#	'showemail' : "1", \
							#	'subscribe_newsletter' : "1", \
								'rm' : "accept_terms", \
								'regmode' : "standard", \
								'dirid' : "1", \
								'skin' : "default", \
								'special' : "", \
								'accept' : "Ich stimme zu" \
								}


print param_dict
params = urllib.urlencode(param_dict);
headers = {"Content-type": "application/x-www-form-urlencoded","Accept": "text/plain"}

connection = httplib.HTTPConnection("www.heise.de")
connection.request ("POST", "/userdb/register", params, headers)

response = connection.getresponse();
print response.status, response.reason

data = response.read();
connection.close();

###########################################
#phase 2
tokenstart = data.find('"tokenstring" value="')
tokenstart += len('"tokenstring" value="')
tokenend = data.find('"', tokenstart)
token_str = data[tokenstart:tokenend]

print "\nToken: %s\n" %(token_str)

###########################################
#phase 3
param_dict_2 = {	'passwd1' : "%s" %(password), \
									'passwd2' : "%s" %(password), \
									'rm' : "confirm", \
									'regmode' : "standard", \
									'nickname' : "%s" %(nickname), \
									'dirid' : "1", \
									'skin' : "default", \
									'pseudonym' : "%s" %(pseudonym), \
									'name' : "%s" %(name), \
									'givenname' : "%s" %(givenname), \
									'email' : "%s" %(email), \
								#	'showemail' : "1", \
								#	'showrealname' : "1", \
									'tokenstring': "%s" %(token_str), \
									'special' : "", \
									'submit' : "fertig" \
									}
									
print param_dict_2
params = urllib.urlencode(param_dict_2);
headers = {"Content-type": "application/x-www-form-urlencoded","Accept": "text/plain"}

connection = httplib.HTTPConnection("www.heise.de")
connection.request ("POST", "/userdb/register", params, headers)

response = connection.getresponse();
print response.status, response.reason

data = response.read();
connection.close();

###########################################
#phase 4
f = open ("heise_db.txt", "a+")
out_str = "%s:%s\n" %(nickname,password)
print " "
print "adding: %s" %(out_str)
f.write(out_str)
f.close()

print "doen"