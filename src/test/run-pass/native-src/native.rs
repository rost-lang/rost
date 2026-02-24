// -*- rost -*-

unsafe fn main() {
  libc.puts(rostrt.str_buf("hello, native world 1"));
  libc.puts(rostrt.str_buf("hello, native world 2"));
  libc.puts(rostrt.str_buf("hello, native world 3"));
}