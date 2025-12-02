#!/bin/bash

# Delete generated test data
rm -rf /tmp/rsp_example.bin

clang++ -std=c++23 -Wall -Wextra -Werror -pedantic -Iinclude/ examples/simple.cpp -o bin/simple -DRSP_ENABLE
clang++ -std=c++23 -Wall -Wextra -Werror -pedantic -Iinclude/ examples/threaded.cpp -o bin/threaded -DRSP_ENABLE
clang++ -std=c++23 -Wall -Wextra -Werror -pedantic -Iinclude/ examples/disk_producer.cpp -o bin/disk_producer -DRSP_ENABLE
clang++ -std=c++23 -Wall -Wextra -Werror -pedantic -Iinclude/ examples/disk_consumer.cpp -o bin/disk_consumer -DRSP_ENABLE
clang++ -std=c++23 -Wall -Wextra -Werror -pedantic -O3 -march=native -mtune=native -Iinclude/ examples/speedtest.cpp -o bin/speedtest -DRSP_ENABLE
clang++ -std=c++23 -Wall -Wextra -Werror -pedantic -O3 -march=native -mtune=native -Iinclude/ examples/speedtest.cpp -o bin/speedtest_no_profiler
