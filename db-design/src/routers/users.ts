import { mongoQuery } from "@/mongo";
import { mongoUser } from "@/mongo/schema";
import { pgUser } from "@/postgres/schema";
import { HTTP_PAGE_SIZE } from "@/utils/constants";
import { zValidator } from "@hono/zod-validator";
import { Hono } from "hono";

const usersRouter = new Hono();

usersRouter.get("/", async ctx => {
  const { page: pageStr } = ctx.req.query();
  let page = parseInt(pageStr ?? "0");

  if (isNaN(page)) page = 0;

  const data = await mongoQuery(() =>
    mongoUser
      .find({}, { _id: 0, __v: 0 })
      .skip(page * HTTP_PAGE_SIZE)
      .limit(HTTP_PAGE_SIZE),
  );

  return ctx.json(data);
});

usersRouter.get("/:id", async ctx => {
  const id = ctx.req.param("id");
  const data = await mongoQuery(() =>
    mongoUser.findOne({ id }, { _id: 0, __v: 0 }),
  );
  if (data === null) return ctx.json({ message: "Not found" }, 404);
  return ctx.json(data);
});

const schema = pgUser.zod;

usersRouter.post("/", zValidator("json", schema), async ctx => {
  const data = ctx.req.valid("json");
  await mongoQuery(() => mongoUser.create(data));
  return ctx.json({}, 204);
});

usersRouter.put("/:id", zValidator("json", schema.partial()), async ctx => {
  const id = ctx.req.param("id");
  const data = ctx.req.valid("json");
  const res = await mongoQuery(() => mongoUser.updateOne({ id }, data));
  if (res.modifiedCount === 0) return ctx.json({ message: "Not found" }, 404);
  return ctx.json({}, 204);
});

usersRouter.delete("/:id", async ctx => {
  const id = ctx.req.param("id");
  const res = await mongoQuery(() => mongoUser.deleteOne({ id }));
  if (res.deletedCount === 0) return ctx.json({ message: "Not found" }, 404);
  return ctx.json({}, 204);
});

export default usersRouter;
