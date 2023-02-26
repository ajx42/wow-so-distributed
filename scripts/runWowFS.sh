#!/bin/bash

./scripts/cleanup.sh
rm /tmp/logs.unreliable.txt

tmux new-session -d -s my_session
tmux split-window -h

tmux send-keys -t 0 "../build/wowRPC/server" C-m

tmux split-window -v -t 0

tmux send-keys -t 1 "./scripts/setupFS.sh" C-m

sleep 2

tmux send-keys -t 2 "cd /tmp/wowfs" C-m

tmux select-pane -t 2

tmux attach -t my_session
