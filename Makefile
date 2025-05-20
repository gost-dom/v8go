fetch:
	git fetch origin
	git fetch upstream

update-all: fetch update-auto-updater update-module-rename update-typeof update-external-support update-developer-docs update-esm-support readme

update-auto-updater:
	git co auto-updater
	git pull
	git rebase upstream/master
	git push -f

update-module-rename:
	git co rename-module-to-gost
	git pull
	git rebase upstream/master
	git push -f

update-typeof:
	git co value-typeof
	git pull
	git rebase upstream/master
	git push -f

update-externa-support:
	git co external-support
	git pull
	git rebase upstream/master
	git push -f

update-developer-docs:
	git co developer-docs
	git pull
	git rebase upstream/master
	git push -f

update-esm-support:
	git co feat/experimental-support-rebased
	git pull
	git rebase upstream/master
	git push -f

update-readme:
	git co readme
	git pull
	git rebase upstream/master
	git push -f
