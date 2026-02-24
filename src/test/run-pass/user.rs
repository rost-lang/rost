// -*- rost -*-

use std (name = "std",
         url = "http://rost-lang.org/src/std",
         uuid = _, ver = _);

fn main() {
  auto s = std._str.alloc(10);
  s += "hello ";
  log s;
  s += "there";
  log s;
  auto z = std._vec.alloc[int](10);
}