for id in $(ipcs -s | awk '/0x/{print $2}'); do ipcrm -s $id; done; \
for id in $(ipcs -m | awk '/0x/{print $2}'); do ipcrm -m $id; done; \
for id in $(ipcs -q | awk '/0x/{print $2}'); do ipcrm -q $id; done
