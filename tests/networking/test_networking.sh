#!/bin/bash

set -u -o pipefail

server=$1
client=$2

# Start the server
${server} 2> /dev/null &
server_pid=$!
sleep 1
kill -0 ${server_pid} 2> /dev/null
running=$?
if [[ ${running} -ne 0 ]]; then
    echo "Failed to start server"
fi

# Start the client
${client}
result=$?

# Kill the server
kill -SIGTERM ${server_pid} 2> /dev/null

# Exit with the result of the client
exit ${result}
