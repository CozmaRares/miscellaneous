import { mongoQuery } from "@/mongo";
import { mongoTimeControl } from "@/mongo/schema";
import { pgTimeControl } from "@/postgres/schema";
import { HTTP_PAGE_SIZE } from "@/utils/constants";
import { zValidator } from "@hono/zod-validator";
import { Hono } from "hono";

const timeControlsRouter = new Hono();

timeControlsRouter.get("/", async ctx => {
  const { page: pageStr } = ctx.req.query();
  let page = parseInt(pageStr ?? "0");

  if (isNaN(page)) page = 0;

  const data = await mongoQuery(() =>
    mongoTimeControl
      .find({}, { _id: 0, __v: 0 })
      .skip(page * HTTP_PAGE_SIZE)
      .limit(HTTP_PAGE_SIZE),
  );

  return ctx.json(data);
});

timeControlsRouter.get("/:id", async ctx => {
  const id = ctx.req.param("id");
  const data = await mongoQuery(() =>
    mongoTimeControl.findOne({ id }, { _id: 0, __v: 0 }),
  );
  if (data === null) return ctx.json({ message: "Not found" }, 404);
  return ctx.json(data);
});

const schema = pgTimeControl.zod;

timeControlsRouter.post("/", zValidator("json", schema), async ctx => {
  const data = ctx.req.valid("json");
  await mongoQuery(() => mongoTimeControl.create(data));
  return ctx.json({}, 204);
});

timeControlsRouter.put(
  "/:id",
  zValidator("json", schema.partial()),
  async ctx => {
    const id = ctx.req.param("id");
    const data = ctx.req.valid("json");
    const res = await mongoQuery(() =>
      mongoTimeControl.updateOne({ id }, data),
    );
    if (res.modifiedCount === 0) return ctx.json({ message: "Not found" }, 404);
    return ctx.json({}, 204);
  },
);

timeControlsRouter.delete("/:id", async ctx => {
  const id = ctx.req.param("id");
  const res = await mongoQuery(() => mongoTimeControl.deleteOne({ id }));
  if (res.deletedCount === 0) return ctx.json({ message: "Not found" }, 404);
  return ctx.json({}, 204);
});

export default timeControlsRouter;
