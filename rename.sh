files=$(grep libbeep * -Rl | xargs)

for i in $files; do
	echo "==> $i"
	sed s:beep\/:audacious\/:g < $i > $i.new
	mv $i.new $i
done;
