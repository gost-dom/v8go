update-all: fetch update-auto-updater update-module-rename update-typeof update-developer-docs update-esm update-handlers update-readme
	git co upstream/master
	git co -b tmp-merge
	git merge rename-module-to-gost value-typeof

fetch:
	git fetch origin
	git fetch upstream

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
	git co support-for-embedded-objects
	git pull
	git rebase upstream/master
	git push -f

update-developer-docs:
	git co developer-docs
	git pull
	git rebase upstream/master
	git push -f

update-esm: update-external-support 
	git co update/esm
	git pull
	git rebase support-for-embedded-objects
	git push -f

update-handlers:
	git co update/handler-support
	git pull
	git rebase upstream/master
	git push -f

update-readme:
	git co readme
	git pull
	git rebase upstream/master
	git push -f
