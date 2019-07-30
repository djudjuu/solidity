contract C {
  function doNotAskForSevens(uint x) public pure returns(bool) {
    require(x != 7, "You must NOT ask for seven");
    assert(x != 77);
    if (x == 777) { revert(); }
    return true;
  }
}
