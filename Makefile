all: submission

# We need to setup the git identity inside Docker since it shouldn't already be setup
# That way, we can make commits, which is needed to run the git patch
# The git-setup.sh script does this
submission:
	./git_setup.sh
	cd xv6-public && \
		git add . && \
		git commit -m "Submission" && \
		git format-patch b818915f793cd20c5d1e24f668534a9d690f3cc8 --stdout > submission.patch
	cd xv6-public && zip ../gradescope-submission.zip submission.patch