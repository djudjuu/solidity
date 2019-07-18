contract C {
    mapping(address => address payable) public m;
    function f(address payable arg) public returns (address payable r) {
        address payable a = m[arg];
        r = arg;

        // breaking: produces a wrong referenced Declaration in
        // AST-node of [functioncall][arguments][Identifier]
        /* address c = address(this); */

        // workaround:
        address c;
        m[c] = address(0);
    }
}
