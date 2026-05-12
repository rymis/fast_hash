
REPO=https://github.com/JacksonAllan/c_cpp_hash_tables_benchmark.git
ROOT=$(cd $(dirname "$0"); pwd)

cd "$ROOT"

die() {
    echo "Error: $*" 1>&2
    exit 1
}

set -eux

# 1. Initialize repository
W="$ROOT/workdir"
[ -d "$W" ] || mkdir "$W"

BENCH="$W/c_cpp_hash_tables_benchmark"

if [ \! -d "$BENCH" ]; then 
    (cd $W; git clone "$REPO")
fi

# 2. Patch repo to use our library (if not patched yet)
if [ \! -f .patched ]; then
    echo Patch
    echo '#define SHIM_16 cf_hash' >> "$W/config.h"
    mkdir -p "$W/shims/cf_hash"
    cp shim.h "$W/shims/cf_hash/"
    touch .patched
fi

# 3. Build project
(cd "$BENCH"; g++ -o benchmark -I. -std=c++20 -static -O3 -DNDEBUG -Wall -Wpedantic main.cpp)
