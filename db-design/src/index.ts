import { Hono } from "hono";
import { logger as honoLogger } from "hono/logger";
import logger from "./utils/logger";
import runWorker from "./worker";
import { env } from "./env";
import usersRouter from "./routers/users";
import timeControlsRouter from "./routers/timeControls";
import openingsRouter from "./routers/openings";
import gameToUserRouter from "./routers/gameToUser";
import gamesRouter from "./routers/games";

if (process.argv.length === 3 && process.argv[2] === "sync") {
  await runWorker(null);
  process.exit(0);
}

const worker = new Worker("src/worker.ts");
worker.onmessage = event => {
  console.log(event.data);
};

const app = new Hono({ strict: false });
app.use(honoLogger((...args) => logger.info(...args)));

app.use("*", async (ctx, next) => {
  try {
    await next();
  } catch (e) {
    logger.error(e);
    return ctx.text("Internal server error", 500);
  }
});

app.route("/users", usersRouter);
app.route("/games", gamesRouter);
app.route("/openings", openingsRouter);
app.route("/time-controls", timeControlsRouter);
app.route("/game-to-user", gameToUserRouter);

export default {
  fetch: app.fetch,
  port: env.PORT,
};

worker.postMessage("");
