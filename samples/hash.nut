/*
*
* Original Javascript version by David Hedbor(http://www.bagley.org/~doug/shootout/)
*
*/
local i, c = 0;
local n;

if(ARGS.len()!=0)
{
  n = ARGS[0].tointeger();
  if(n < 1) n = 1;
} else {   
  n = 1;
}

local X =  {};
for (i=1; i<=n; i=i+1) {
   X[i.tostring()] <- i;
   
}
for (i=n; i>0; i=i-1) {
  if (X[i.tostring()]) c=c+1;
  
}

print("\n#####c="+c+"#######\n");

