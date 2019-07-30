contract C {
  event E (uint x);
  function f () public {
    emit E (8);
  }
}
