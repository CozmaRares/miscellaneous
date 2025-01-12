import { mongoQuery } from "@/mongo";
import { mongoGameOpening as mongoOpening } from "@/mongo/schema";
import { pgGameOpening as pgOpening } from "@/postgres/schema";
import { HTTP_PAGE_SIZE } from "@/utils/constants";
import { zValidator } from "@hono/zod-validator";
import { Hono } from "hono";

const openingsRouter = new Hono();

openingsRouter.get("/", async ctx => {
  const { page: pageStr } = ctx.req.query();
  let page = parseInt(pageStr ?? "0");

  if (isNaN(page)) page = 0;

  const data = await mongoQuery(() =>
    mongoOpening
      .find({}, { _id: 0, __v: 0 })
      .skip(page * HTTP_PAGE_SIZE)
      .limit(HTTP_PAGE_SIZE),
  );

  return ctx.json(data);
});

openingsRouter.get("/:eco", async ctx => {
  const eco = ctx.req.param("eco");
  const data = await mongoQuery(() =>
    mongoOpening.findOne({ eco }, { _id: 0, __v: 0 }),
  );
  if (data === null) return ctx.json({ message: "Not found" }, 404);
  return ctx.json(data);
});

const schema = pgOpening.zod;

openingsRouter.post("/", zValidator("json", schema), async ctx => {
  const data = ctx.req.valid("json");
  await mongoQuery(() => mongoOpening.create(data));
  return ctx.json({}, 204);
});

openingsRouter.put("/:eco", zValidator("json", schema.partial()), async ctx => {
  const eco = ctx.req.param("eco");
  const data = ctx.req.valid("json");
  const res = await mongoQuery(() => mongoOpening.updateOne({ eco }, data));
  if (res.modifiedCount === 0) return ctx.json({ message: "Not found" }, 404);
  return ctx.json({}, 204);
});

openingsRouter.delete("/:eco", async ctx => {
  const eco = ctx.req.param("eco");
  const res = await mongoQuery(() => mongoOpening.deleteOne({ eco }));
  if (res.deletedCount === 0) return ctx.json({ message: "Not found" }, 404);
  return ctx.json({}, 204);
});

export default openingsRouter;
