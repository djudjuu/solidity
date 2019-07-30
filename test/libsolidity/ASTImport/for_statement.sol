contract C {
  function countDownFrom(uint x) public {
    uint j;
    for (uint i=x; i>0; i--) {
      j+= i;
      if (j == 10) {
        break;
      } else {
        continue;
      }
    }
  }
}
