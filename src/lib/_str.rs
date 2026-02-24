import rostrt.sbuf;

native "rost" mod rostrt {
  type sbuf;
  fn str_buf(str s) -> sbuf;
  fn str_len(str s) -> uint;
  fn str_alloc(int n_bytes) -> str;
}

fn is_utf8(vec[u8] v) -> bool {
}

fn alloc(int n_bytes) -> str {
  ret rostrt.str_alloc(n_bytes);
}

fn len(str s) -> uint {
  ret rostrt.str_len(s);
}

fn buf(str s) -> sbuf {
  ret rostrt.str_buf(s);
}