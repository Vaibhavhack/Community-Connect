// SelfTest.h
// -----------------------------------------------------------------------------
// Purpose : A one-command verification of the Stage 2 foundation.
//
// Run it on your MacBook with:
//
//     ./build/cc-server --selftest
//
// It exercises the parts that are easy to get subtly wrong and that a console
// demo would not catch:
//
//   1. escaping round-trip  - "|", "\" and newlines survive a save/load cycle
//   2. password hashing     - plain text never stored, wrong password rejected,
//                             unique salt per user (same password -> different hash)
//   3. validation           - bad email, short password, duplicate email,
//                             duplicate username (case-insensitive) all rejected
//   4. persistence          - data survives a NEW UserService instance
//                             (this is the "close and reopen the app" requirement)
//   5. authentication       - login, token lookup, logout invalidation
//   6. profile editing      - empty field = unchanged, password change needs
//                             the current password
//
// Everything runs inside build/selftest-data/, never inside your real data/.
// -----------------------------------------------------------------------------

#ifndef CC_SELF_TEST_H
#define CC_SELF_TEST_H

namespace cc {

// Returns the number of failures. 0 means every check passed.
int runSelfTest();

} // namespace cc

#endif // CC_SELF_TEST_H
