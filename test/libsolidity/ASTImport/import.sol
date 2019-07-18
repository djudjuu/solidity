import "./basicContracts.sol";
import "./basicContracts.sol" as basicContracts;
import * as base from "./basicContracts.sol";

// breaking => fatal error in testCaseCreator
/* import { C as basicC, D} from "./basicContracts.sol"; */
/* import { C as basicC} from "./basicContracts.sol"; */
/* import { D } from "./basicContracts.sol"; */

contract A1 is A {}
contract B1 is basicContracts.B {}

/* contract C1 is basicC {} */
/* contract C2 is D {} */

// breaking! first and last element are switched in AST-Node baseContracts
/* contract ABCD is base.A, base.B, base.C, base.D {} */
contract ABCD is base.A {}
