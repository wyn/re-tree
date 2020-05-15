open BsMocha.Mocha;
module Assert = BsMocha.Assert;
open Test_utils;

describe("subtrees", () => {
  let t = StandardTree.t;
  let t1 =
    t
    ->M.getSubtree(StandardTree.path1->P.moveUp, ID.create("a"))
    ->Option.getExn;
  it("gotSubtree", () => {
    t1->M.myId == ID.create("a") |> Assert.equal(true)
  });

  it("gotSubtreeIds", () => {
    [%log.debug "t1 subtree:" ++ t1->M.toString; ("", "")];
    let cids = t1->M.getAllIds;
    cids->Array.size |> Assert.equal(2);
  });

  it("gotOneChildInSubtree", () => {
    t1->M.children->Map.size |> Assert.equal(1)
  });

  it("gotChildInSubtree", () => {
    t1->M.children->Map.has(CID.create("child1")) |> Assert.equal(true)
  });

  let t1 =
    t
    ->M.getSubtree(StandardTree.path1->P.moveUp->P.moveUp, ID.create("1"))
    ->Option.getExn;
  it("gotSubtree2", () => {
    t1->M.myId == ID.create("1") |> Assert.equal(true)
  });

  it("gotTwoChildInSubtree", () => {
    t1->M.children->Map.size |> Assert.equal(2)
  });

  it("gotChildInSubtree1", () => {
    t1->M.children->Map.has(CID.create("a")) |> Assert.equal(true)
  });
  it("gotChildInSubtree2", () => {
    t1->M.children->Map.has(CID.create("b")) |> Assert.equal(true)
  });
});

describe("addSubtree", () => {
  let t = StandardTree.t;
  let t2 = StandardTree.t2;

  [%log.debug "t2: " ++ t2->M.toString; ("", "")];

  let t3 = t->M.addSubtree(ID.create("test"), StandardTree.path3, t2);
  [%log.debug "t3: " ++ t3->M.toString; ("", "")];
  let t4 =
    t3
    ->M.getSubtree(P.fromRootToPathList(["2", "c"]), ID.create("test"))
    ->Option.getExn;
  it("notAddedNewRootNode", () => {
    t4->M.isRoot |> Assert.equal(false)
  });
  it("hasAddedNodeWithCorrectId", () => {
    t4->M.myId == ID.create("test") |> Assert.equal(true)
  });
  let _get = (p1, p2) => {
    t4
    ->M.children
    ->Map.getExn(CID.create(p1))
    ->M.children
    ->Map.getExn(CID.create(p2))
    ->M.children;
  };

  it("3aHasTwoChildren", () => {
    _get("3", "a")->Map.size |> Assert.equal(2)
  });

  it("3aHasChild4", () => {
    _get("3", "a")->Map.has(CID.create("child4")) |> Assert.equal(true)
  });

  it("3aHasChild5", () => {
    _get("3", "a")->Map.has(CID.create("child5")) |> Assert.equal(true)
  });

  it("3bHasOneChild", () => {
    _get("3", "b")->Map.size |> Assert.equal(1)
  });

  it("3bHasChild6", () => {
    _get("3", "b")->Map.has(CID.create("child6")) |> Assert.equal(true)
  });
});

describe("removeSubtree", () => {
  let t = StandardTree.t;

  let t1 =
    t->M.removeSubtree(
      StandardTree.path1->P.moveUp->P.moveUp,
      CID.create("1"),
    );
  let t2 = t1->M.children->Map.getExn(CID.create("2"));
  it("oneSubtreeRemoved", () => {
    t2->M.children->Map.size |> Assert.equal(1)
  });

  it("subtreeIsNotInChildrenCollection", () => {
    t2->M.children->Map.has(ID.create("1")->I.convertFocusToChild)
    |> Assert.equal(false)
  });

  it("subtree2IsStillInChildrenCollection", () => {
    t2->M.children->Map.has(ID.create("c")->I.convertFocusToChild)
    |> Assert.equal(true)
  });
});
