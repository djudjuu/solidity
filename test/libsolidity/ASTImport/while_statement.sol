contract C {
  function countDownFrom(uint x) public {
    uint j = x;
    while (j > 0) {
      j--;
    }
    uint jj = x;
    do {
      jj--;
    }
    while (j > 0);
  }
}
