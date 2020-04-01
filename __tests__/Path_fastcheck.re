open BsMocha.Mocha;
open BsFastCheck.Arbitrary;
open BsFastCheck.Property.Sync;
open BsFastCheck.Arbitrary.Combinators;

module M = Path.T;

let idString = stringWithLength(1, 10); // PID need to be not empty

describe("construction", () => {
  it("fromPathToRoot is reverse fromRootToPath", () => {
    assertProperty1(
      list(idString),
      ls => {
        let pth1 = M.fromPathToRootList(ls);
        let pth2 = M.fromRootToPathList(ls->List.reverse);
        M.eq(pth1, pth2);
      },
    )
  });

  it("path is same size as input list", () => {
    assertProperty1(
      list(idString),
      ls => {
        let n = M.fromList(ls)->M.size;
        let m = List.size(ls);
        n == m;
      },
    )
  });

  it("concat two path is same as making from concat lists", () => {
    assertProperty2(
      list(idString),
      list(idString),
      (ls1, ls2) => {
        let pth1 = M.fromList(ls1);
        let pth2 = M.fromList(ls2);
        let pth3 = M.fromList(List.concat(ls1, ls2));
        M.eq(pth3, M.concat(pth1, pth2));
      },
    )
  });
});

describe("append-remove", () => {
  it("append-remove same as starting", () => {
    assertProperty2(
      idString,
      list(idString),
      (s, ls_) => {
        let ls = ls_->List.keep(l => l != s);
        let pid = Path.PID.create(s);
        //        [%log.debug pid->Path.PID.toString; ("", "")];
        let pth1 = M.fromList(ls)->M.append(pid)->M.removeElement(pid);
        //        [%log.debug pth1->M.toString; ("", "")];
        let pth2 = M.fromList(ls);
        //        [%log.debug pth2->M.toString; ("", "")];
        let e = M.eq(pth1, pth2);
        /* [%log.debug */
        /*   "s: " */
        /*   ++ s */
        /*   ++ ", ls: [" */
        /*   ++ (ls |> String.concat(",")) */
        /*   ++ "] is ok: " */
        /*   ++ e->string_of_bool; */
        /*   ("", "") */
        /* ]; */
        e;
      },
    )
  });

  it("remove-append same as starting", () => {
    assertProperty2(
      idString,
      list(idString),
      (s, ls_) => {
        let ls = ls_->List.keep(l => l != s);
        let pid = Path.PID.create(s);
        //        [%log.debug pid->Path.PID.toString; ("", "")];
        let pth1 =
          M.fromList([s, ...ls])->M.removeElement(pid)->M.append(pid);
        //        [%log.debug pth1->M.toString; ("", "")];
        let pth2 = M.fromList([s, ...ls]);
        //        [%log.debug pth2->M.toString; ("", "")];
        let e = M.eq(pth1, pth2);
        /* [%log.debug */
        /*   "s: " */
        /*   ++ s */
        /*   ++ ", ls: [" */
        /*   ++ (ls |> String.concat(",")) */
        /*   ++ "] is ok: " */
        /*   ++ e->string_of_bool; */
        /*   ("", "") */
        /* ]; */
        e;
      },
    )
  });

  it("remove removes all occurances", () => {
    assertProperty4(
      idString,
      list(idString),
      list(idString),
      list(idString),
      (s, ls1, ls2, ls3) => {
        let ls1 = ls1->List.keep(l => l != s);
        let ls2 = ls2->List.keep(l => l != s);
        let ls3 = ls3->List.keep(l => l != s);
        let pid = Path.PID.create(s);
        //        [%log.debug pid->Path.PID.toString; ("", "")];
        let pth1 =
          M.fromList(List.concatMany([|ls1, [s], ls2, [s], ls3|]))
          ->M.removeElement(pid);
        //        [%log.debug pth1->M.toString; ("", "")];
        let pth2 = M.fromList(List.concatMany([|ls1, ls2, ls3|]));
        //        [%log.debug pth2->M.toString; ("", "")];
        let e = M.eq(pth1, pth2);
        e;
      },
    )
  });
});

describe("root/parent/moveup", () => {
  it("root of non empty is always the same", () => {
    assertProperty2(
      idString,
      list(idString),
      (s, ls) => {
        // NOTE root to path  here
        let pth = M.fromRootToPathList([s, ...ls]);
        let root = pth->M.root->Option.getExn;
        root->Path.PID.toString == s;
      },
    )
  });

  it("parent of non empty is always next one", () => {
    assertProperty2(
      idString,
      list(idString),
      (s, ls) => {
        // NOTE path to root here
        let pth = M.fromPathToRootList([s, ...ls]);
        let parent = pth->M.parent->Option.getExn;
        let e = parent->Path.PID.toString == s;
        /* [%log.debug */
        /*   pth->M.toString */
        /*   ++ " with parent: " */
        /*   ++ parent->Path.PID.toString */
        /*   ++ " is ok " */
        /*   ++ e->string_of_bool; */
        /*   ("", "") */
        /* ]; */
        e;
      },
    )
  });

  it("moveUp of non empty is always rest of path", () => {
    assertProperty2(
      idString,
      list(idString),
      (s, ls) => {
        // NOTE path to root here
        let pth = M.fromPathToRootList([s, ...ls]);
        let pth1 = M.fromPathToRootList(ls);
        let pathUp = pth->M.moveUp;
        let e = M.eq(pth1, pathUp);
        /* [%log.debug */
        /*   pth->M.toString */
        /*   ++ " with parent: " */
        /*   ++ parent->Path.PID.toString */
        /*   ++ " is ok " */
        /*   ++ e->string_of_bool; */
        /*   ("", "") */
        /* ]; */
        e;
      },
    )
  });
});
