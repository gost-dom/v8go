update-all: update-strict-equals # fetch update-auto-updater update-module-rename update-typeof update-developer-docs update-esm update-handlers update-readme
	git co upstream/master
	git merge \
		rename-module-to-gost \
		value-typeof \
		update/esm \
		update/handler-support \
		add-value-strict-equals
	go test

fetch:
	git fetch origin
	git fetch upstream

update-auto-updater:
	# git co auto-updater
	git pull --rebase
	git rebase upstream/master
	git push -f

update-module-rename:
	git co rename-module-to-gost
	# git pull --rebase
	git rebase upstream/master
	git push -f

update-typeof:
	git co value-typeof
	# git pull --rebase
	git rebase upstream/master
	git push -f

update-externa-support:
	git co support-for-embedded-objects
	# git pull --rebase
	git rebase upstream/master
	git push -f

update-developer-docs:
	git co developer-docs
	# git pull --rebase
	git rebase upstream/master
	git push -f

update-esm: update-external-support 
	git co update/esm
	# git pull --rebase
	git rebase upstream/master
	git push -f

update-handlers:
	git co update/handler-support
	# git pull --rebase
	git rebase support-for-embedded-objects
	git push -f

update-strict-equals:
	git co add-value-strict-equals
	git pull --rebase
	git rebase upstream/master
	git push -f


update-readme:
	git co readme
	# git pull
	git rebase upstream/master
	git push -f
