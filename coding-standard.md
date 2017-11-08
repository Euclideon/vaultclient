# Euclideon Vault Client Coding Standard

## Generally

- 2-space indents, no tabs
- `camelCase` variables
- `ProperCase` function/methods
- Start pointers with a lower case p (eg `void *pBlockData`) (clarification at the bottom of this document)
  - pointers to pointers have two p's (`**ppData`) and so on.
- `m_` prefix for class members
- `s_` prefix for statics
- `g_` for globals or `extern` (they should be avoided)
- Structure members should not be prefixed.
- Keep the code compact, with white-space used to delineate groups of functionally related lines of code.
- No braces on single list ifs but braces for single-line if/else's when either the if or the else requires them
- Try to avoid functions using more than a couple of pages.
- Keep the while from a do-while with its closing brace so it doesn't look like a while.
- Space after a comma, not around parentheses.
- The asterisk/ampersand to denote a pointer/reference goes with the variable, not the type
- Always try to use `++`/`--` pre-versions where possible.
- Avoid incrementing/decrementing variables in function calls, the extra line(s) are worth the clarity
- Use struct where possible. It should be a class if it has non-public members OR a constructor OR virtual functions.
- Avoid member functions except where it creates more work to avoid them
- Functions should be prefixed by the name of the module followed by an underscore
- `Create` or `Init` functions that take a `**pp` will alloc the type and the `Deinit` and `Destroy` functions will dealloc or recycle and set as nullptr
- Generally favour exactly one return statement, and goto or branches for each error check

## Memory
- Treat accessing memory like it's a hard drive, the hard drive like it's the internet, and the internet like it's the post office.
- Allocate memory infrequently, and deal with failures
- Use `udAlloc` when you have an amount of memory to allocate
- Use `udAllocType` when you have a type to allocate

### Variable names
- All variables follow the variable naming standard with the following exceptions:
  - Uniforms are prefixed with a lower case `u` Eg. `uniform vec3 uColour`
  - Variables passed between passes (eg Vertex to Fragment) are prefixed with lower case `v` Eg. `out vec2 vUVs` on both sides (for Varying)
  - Fragment out variables are prefixed with a lower case `o` Eg. `out vec4 oFinalColour` (For out)

Some clarity for `p` confusion
- `T a;` //Never P
- `T a[] = {...};` // Never P
- `T *a[] = {...};` // Never P
- `T a[7];` // Only acceptable for stack variables- members and globals always have a define/enum
- `T a[Enum];` // Never P
- `T *a[Enum];` // Never P
- `T **a[Enum];` // Never P
- `T *pA;` // Always P
- `T **ppA;` // Always PP
- `T ***pppA;` // Always PPP
