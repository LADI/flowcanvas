all:
	@echo "Targets: upload-docs"

upload-docs:
	rsync -avz --delete -e ssh build/flowcanvas/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/flowcanvas
	rsync -avz --delete -e ssh build/raul/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/raul
	rsync -avz --delete -e ssh build/redlandmm/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/redlandmm
	rsync -avz --delete -e ssh build/slv2/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/slv2

# Doesn't work
deb:
	for i in raul slv2 patchage; do \
		cd $i; \
		if [ -e debian ]; then \
			echo "ERROR: ./debian exists, you must move it out of the way."; \
		else \
			ln -s debian-sid debian; \
			dpkg-buildpackage -sn; \
			rm debian; \
		fi; \
	done
