UPSTREAM_BASE=v0.38.0
GO=CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go

# update-typeof
update-all: fetch update-handlers update-set-prototypes update-auto-updater update-module-rename update-developer-docs update-esm update-readme
	git co $(UPSTREAM_BASE)
	git merge --rerere-autoupdate --no-edit \
		auto-updater \
		update/handler-support
	git merge --rerere-autoupdate --no-edit \
		update/esm || ./check-rerere
	# git merge --rerere-autoupdate --no-edit \
	# 	value-typeof || ./check-rerere
	git merge --rerere-autoupdate --no-edit \
		rename-module-to-gost || ./check-rerere
	git merge --rerere-autoupdate --no-edit \
		update/set-prototypes || ./check-rerere
	sed -i "" 's/tommie/gost-dom/g' *_test.go
	git commit -a --amend --no-edit
	go fmt
	go generate
	git diff --exit-code
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go vet
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go test
	git push origin HEAD:master -f

fetch:
	git fetch origin
	# git fetch upstream

update-auto-updater:
	git co auto-updater
	git branch -f backup/auto-updater
	# git pull --rebase
	git rebase $(UPSTREAM_BASE)
	go fmt
	go generate
	git diff --exit-code
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go vet
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go test
	git push -f

update-module-rename:
	git co rename-module-to-gost
	git branch -f backup/rename-module
	# git pull --rebase
	git rebase $(UPSTREAM_BASE)
	go fmt
	go generate
	git diff --exit-code
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go vet
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go test
	git push -f

# update-typeof:
# 	git co value-typeof
# 	# git pull --rebase
# 	git rebase $(UPSTREAM_BASE)
# 	go fmt
# 	go generate
# 	git diff --exit-code
# 	go vet
# 	go test
# 	# git push -f

update-external-support:
	git co support-for-embedded-objects
	git branch -f backup/externals
	# git pull --rebase
	git rebase $(UPSTREAM_BASE)
	go fmt
	go generate
	git diff --exit-code
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go vet
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go test
	git push -f

update-developer-docs:
	git co developer-docs
	git branch -f backup/developer-docs
	# git pull --rebase
	git rebase $(UPSTREAM_BASE)
	go fmt
	go generate
	git diff --exit-code
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go vet
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go test
	git push -f

update-esm:
	git co update/esm
	git branch -f backup/esm
	# git pull --rebase
	git rebase $(UPSTREAM_BASE)
	go fmt
	go generate
	git diff --exit-code
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go vet
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go test
	# git push -f

update-handlers:
	git co update/handler-support
	git branch -f backup/handler-support
	# git pull --rebase
	git rebase $(UPSTREAM_BASE)
	go fmt
	go generate
	git diff --exit-code
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go vet
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go test
	git push -f

update-set-prototypes:
	git co update/set-prototypes
	git branch -f backup/set-prototypes
	# git pull --rebase
	git rebase $(UPSTREAM_BASE)
	go fmt
	go generate
	git diff --exit-code
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go vet
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go test
	git push -f

update-readme:
	git co readme
	git co -f backup/readme
	# git pull
	git rebase $(UPSTREAM_BASE)
	go fmt
	go generate
	git diff --exit-code
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go vet
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go test
	git push -f

.PHONY: test
test:
	CC=clang CXX=clang++ CGO_CXXFLAGS=-nostdinc++ go test
