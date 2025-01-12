import { mongoQuery } from "@/mongo";
import { mongoGameToUser as mongoGU } from "@/mongo/schema";
import { pgGameToUser as pgGU } from "@/postgres/schema";
import { HTTP_PAGE_SIZE } from "@/utils/constants";
import { zValidator } from "@hono/zod-validator";
import { Hono } from "hono";

const gameToUserRouter = new Hono();

gameToUserRouter.get("/", async ctx => {
  const { page: pageStr } = ctx.req.query();
  let page = parseInt(pageStr ?? "0");

  if (isNaN(page)) page = 0;

  const data = await mongoQuery(() =>
    mongoGU
      .find({}, { _id: 0, __v: 0 })
      .skip(page * HTTP_PAGE_SIZE)
      .limit(HTTP_PAGE_SIZE),
  );

  return ctx.json(data);
});

gameToUserRouter.get("/:game_id/:user_id", async ctx => {
  const { game_id, user_id } = ctx.req.param();
  const data = await mongoQuery(() =>
    mongoGU.findOne({ game_id, user_id }, { _id: 0, __v: 0 }),
  );
  if (data === null) return ctx.json({ message: "Not found" }, 404);
  return ctx.json(data);
});

const schema = pgGU.zod;

gameToUserRouter.post("/", zValidator("json", schema), async ctx => {
  const data = ctx.req.valid("json");
  await mongoQuery(() => mongoGU.create(data));
  return ctx.json({}, 204);
});

gameToUserRouter.put(
  "/:game_id/:user_id",
  zValidator("json", schema.partial()),
  async ctx => {
    const { game_id, user_id } = ctx.req.param();
    const data = ctx.req.valid("json");
    const res = await mongoQuery(() =>
      mongoGU.updateOne({ game_id, user_id }, data),
    );
    if (res.modifiedCount === 0) return ctx.json({ message: "Not found" }, 404);
    return ctx.json({}, 204);
  },
);

gameToUserRouter.delete("/:game_id/:user_id", async ctx => {
  const { game_id, user_id } = ctx.req.param();
  const res = await mongoQuery(() => mongoGU.deleteOne({ game_id, user_id }));
  if (res.deletedCount === 0) return ctx.json({ message: "Not found" }, 404);
  return ctx.json({}, 204);
});

export default gameToUserRouter;
