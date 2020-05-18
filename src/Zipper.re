module I = Identity;
module ID = I.FocusId;
module PID = I.ParentId;
module CID = I.ChildId;
module P = Path.T;
module type GRAPH = Graph.GRAPH;

module type ZIPPER = {
  type focus;
  type graph('a);
  type t('a);
  let create: (graph('a), focus) => option(t('a));
  let toString: t('a) => string;

  let focus: t('a) => focus;
  let up: t('a) => option(t('a));
  let down: t('a) => option(t('a));
  let left: t('a) => option(t('a));
  let right: t('a) => option(t('a));
  /* let current: t('a) => option(Graph.T.t('a)); */
  /* let context: t('a) => Graph.T.t('a); */
};

module Make =
       (G: GRAPH)
       : (ZIPPER with type focus = ID.t and type graph('a) = G.t('a)) => {
  type focus = ID.t;
  type graph('a) = G.t('a);
  type t('a) = {
    focus_: ID.t,
    up_: option(PID.t),
    down_: list(ID.t),
    left_: list(ID.t),
    right_: list(ID.t),
    background_: G.t('a),
  };

  let create = (g, focus) => {
    g->G.containsId(focus)
      ? {
        let up_ = g->G.parentId(focus);
        let down_ =
          g
          ->G.childIds(focus)
          ->List.sort((x, y) => Pervasives.compare(x, y));
        let siblings =
          up_
          ->Option.map(pid => g->G.childIds(pid->I.convertParentToFocus))
          ->Option.getWithDefault([])
          ->List.sort((x, y) => Pervasives.compare(x, y));
        //              String.compare(x->ID.toString, y->ID.toString)
        //            );
        let index = siblings->List.toArray->Array.getIndexBy(i => i == focus);
        let lr = index->Option.flatMap(i => {siblings->List.splitAt(i)});
        let (left_, right_) =
          lr->Option.mapWithDefault(([], []), ((l, r)) =>
            (l->List.reverse, r->List.tail->Option.getWithDefault([]))
          );
        {focus_: focus, up_, down_, left_, right_, background_: g}->Some;
      }
      : None;
  };

  let toString = t =>
    "Focus: "
    ++ t.focus_->ID.toString
    ++ ", up: "
    ++ t.up_->Option.mapWithDefault("None", PID.toString)
    ++ ", down: ["
    ++ (t.down_->List.map(ID.toString) |> String.concat(","))
    ++ "]"
    ++ ", left: ["
    ++ (t.left_->List.map(ID.toString) |> String.concat(","))
    ++ "]"
    ++ ", right: ["
    ++ (t.right_->List.map(ID.toString) |> String.concat(","))
    ++ "]";
  let focus = t => t.focus_;
  let up = t =>
    t.up_
    ->Option.flatMap(pid =>
        t.background_->create(pid->I.convertParentToFocus)
      );

  let down = t =>
    switch (t.down_) {
    | [hd, ...tl] =>
      {
        ...t,
        focus_: hd,
        left_: [],
        right_: tl,
        up_: t.focus_->I.convertFocusToParent->Some,
        down_:
          t.background_
          ->G.childIds(hd)
          ->List.sort((x, y) => Pervasives.compare(x, y)),
      }
      ->Some
    | [] => t->Some
    };

  let left = t => {
    let (focus_, left_, right_, down_) =
      switch (t.left_) {
      | [hd, ...tl] => (
          hd,
          tl,
          t.right_->List.add(t.focus_),
          t.background_->G.childIds(hd),
        )
      | [] => (t.focus_, t.left_, t.right_, t.down_)
      };
    {...t, focus_, left_, right_, down_}->Some;
  };
  let right = t => {
    let (focus_, left_, right_, down_) =
      switch (t.right_) {
      | [hd, ...tl] => (
          hd,
          t.left_->List.add(t.focus_),
          tl,
          t.background_->G.childIds(hd),
        )
      | [] => (t.focus_, t.left_, t.right_, t.down_)
      };
    {...t, focus_, left_, right_, down_}->Some;
  };
};
