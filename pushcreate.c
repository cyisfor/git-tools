#include <git2/repository.h>
#include <git2/remote.h>
#include <sys/wait.h> // waitpid
#include <assert.h>
#include <unistd.h> // pipe, fork, dup2, close, execlp
#include <error.h>
#include <string.h>

#define repo_check(res) assert(res == 0)

#define ERROR(...) error(23,0, __VA_ARGS__)
#define LITSIZ(a) (sizeof(a)-1)
#define LITLEN(a) a,LITSIZ(a)

int waitfor(int pid) {
	int status;
	int derp = waitpid(pid,&status,0);
	assert(derp == pid);
	assert(WIFEXITED(status));
	return WEXITSTATUS(status);
}

int ssh(int* pid, const char* host) {
	int io[2];
	pipe(io);
	*pid = fork();
	if(*pid == 0) {
		dup2(io[0],0);
		close(io[0]);
		close(io[1]);
		execlp("ssh","ssh","-Y",host,NULL);
		abort();
	}
	close(io[0]);
	return io[1];
}

int main(int argc, char *argv[])
{
	if(argc != 3 && argc != 2) {
		ERROR("pushcreate repo [branch]");
		return 23;
	}
	const char* branch = "pushed";
	size_t branch_len = LITSIZ("pushed");
	if(argc == 3) {
		branch = argv[2];
		branch_len = strlen(branch);
	}

	git_repository* repo = NULL;
	repo_check(git_repository_open_ext(
							 &repo,
							 ".",
							 0,
							 NULL));

	const char* remote_name = argv[1];
	git_remote* remote = NULL;
	const char* url = remote_name;
	size_t ulen = strlen(url);
	if(0 == git_remote_lookup(&remote, repo, remote_name)) {
		url = git_remote_pushurl(remote);
	}

	// assume it's in the form host:dest
	const char* colon = memchr(url,':',ulen);
	if(!colon) {
		ERROR("only one kind of URL supported sorry");
		return 23;
	}
	char host[0x100];
	memcpy(host,url,(colon-url));
	host[colon-url] = '\0';
	const char* path = colon+1; // ~/some/where/repo
	size_t plen = ulen - (path-url);

	int push() {
		int pid = fork();
		if(pid == 0) {
			char derp[0x100] = "HEAD:";
			memcpy(derp+LITSIZ("HEAD:"),branch,branch_len);
			derp[LITSIZ("HEAD:")+branch_len] = '\0';
			execlp("git","git","push",remote_name,derp,NULL);
			abort();
		}
		return waitfor(pid);
	}

	int pid;
	int out = ssh(&pid,host);
	write(out,LITLEN("mkdir -p "));
	write(out,path,plen);
	write(out,LITLEN("\ncd "));
	write(out,path,plen);
	write(out,LITLEN("\nexec git init\n"));
	close(out);

	int status = waitfor(pid);
	assert(status == 0);

	status = push();
	assert(status == 0);
	
	// now merge it remotely
	out = ssh(&pid,host);
	write(out,LITLEN("cd "));
	write(out,path,plen);
	write(out,LITLEN("\ngit merge "));
	write(out,branch,branch_len);
	write(out,LITLEN("\n"));
	write(out,LITLEN("exec git branch -d "));
	write(out,branch,branch_len);
	write(out,LITLEN("\n"));
	close(out);
	status = waitfor(pid);
	assert(status == 0);

	return 0;
}
