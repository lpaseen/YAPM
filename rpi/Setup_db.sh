#!/bins/bash

# Set up the database - assumes it's nothing there

mysqladmin -u root -p  create powermon


#GRANT USAGE ON *.* TO mysqlar@localhost;
mysql -uroot -p mysql -e "grant all on powermon.* to powermon@localhost identified by 'energymon';"

#create two empty tables
mysql -upowermon -penergymon powermon <powermon.sql
