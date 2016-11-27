#!/usr/bin/perl
# Call mtr in out-of-source build
$ENV{MTR_BINDIR} = "/home/jiamin/percona-server/build/5.6-debug";
chdir("/home/jiamin/percona-server/mysql-test");
exit(system($^X, "/home/jiamin/percona-server/mysql-test/mysql-test-run.pl", @ARGV) >> 8);
