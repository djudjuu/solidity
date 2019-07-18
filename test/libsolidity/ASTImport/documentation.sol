// ---- SOURCE: a

/**This contract is empty*/ contract C {}

// ---- SOURCE: b

/**This contract is empty
 and has a line-breaking comment.*/
contract D {}

// ---- SOURCE: c

contract Z {
  /** Some comment on Evt.*/ event Evt();
  /** Some comment on mod.*/ modifier mod() { _; }
  /** Some comment on fn.*/ function fn() public {}
}
