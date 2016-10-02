inotifywait -q -m -e close_write ../../../src/modules |
while read -r filename event; do
	/usr/bin/make         # or "./$filename"
done
