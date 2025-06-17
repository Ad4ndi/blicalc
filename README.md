# blicalc - simple and safe calculator

## Manifetso 

blicalc is designed for educational purposes, with the goal of creating a paranoid secure calculator with its own arithmetic engine. It is also portable, fast and optimized. It is theoretically a replacement for POSIX bc.

## Install

```bash
git clone https://github.com/Ad4ndi/blicalc
cd blicalc
<cc++> blicalc.cpp -o blicalc -O3 -std=c++20
```

(In <cc++> put your compiler name, for example - clang++)

## Why blicalc?

- Easily extendable
- Standard libraries only
- Complex numbers support
- Logic safety
- Type safety
- Memory safety
- Unary operations support
- Abstract syntax tree
- Safe error handling at all stages
- Based on lambda functions
