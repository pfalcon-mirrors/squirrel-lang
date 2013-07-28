/*
*
* Original Javascript version by David Hedbor(http://www.bagley.org/~doug/shootout/)
*
*/
local n = ARGS.len()!=0?ARGS[0].tointeger():1
local hash1 = {};
local hash2 = {};
local arr = [];arr.resize(10000);
local idx;

for (local i=0; i<10000; i=i+1) {
  idx = "foo_"+i;
  hash1[idx] <- i;
  hash2[idx] <- hash1[idx];
}

for (local i = 1; i < n; i=i+1) {
  foreach(a,val in hash1) {
    hash2[a] <- hash2[a]+val;
  }
}

print(hash1["foo_1"]+"\n")
print(hash1["foo_9999"]+"\n")
print(hash2["foo_1"]+"\n")
print(hash2["foo_9999"]+"\n");
