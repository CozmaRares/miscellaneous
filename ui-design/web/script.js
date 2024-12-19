const meals = new Map();

const searchForm = document.querySelector("#search");

async function searchByIngredient(e) {
  e.preventDefault();

  const button = document.querySelector("#search button[type=submit]");
  const loader = document.querySelector("#loader");
  const mealsList = document.querySelector("#meal-list");

  button.setAttribute("disabled", true);
  loader.classList.remove("hidden");
  mealsList.classList.add("hide");

  const ingredient = new FormData(e.target).get("ingredient");

  const fetchPromise = fetcher({
    url: `https://www.themealdb.com/api/json/v1/1/filter.php?i=${ingredient}`,
  });
  const sleepPromise = sleep(1000);

  const [{ status, value }] = await Promise.allSettled([
    fetchPromise,
    sleepPromise,
  ]);

  button.removeAttribute("disabled");
  loader.classList.add("hidden");

  if (status === "rejected") return toast("Failed to fetch");

  searchForm.dataset.center = false;

  meals.clear();

  if (Array.isArray(value.meals))
    value.meals.forEach(({ strMeal, strMealThumb, idMeal }) =>
      meals.set(idMeal, {
        meal: strMeal,
        image: strMealThumb,
      }),
    );

  setTimeout(refreshMeals, 500);
}

function refreshMeals() {
  const list = document.querySelector("#meal-list");
  list.classList.add("hide");
  list.innerHTML = "";

  if (meals.size === 0)
    list.appendChild(
      createElement("li", { style: "text-align: center;" }, "No meals found"),
    );
  else
    meals.entries().forEach(([id, { meal, image }]) =>
      list.appendChild(
        createElement(
          "li",
          {
            className: "meal-item",
          },
          createElement(
            "button",
            {
              onclick: () => showMealDetails(id),
            },
            [
              createElement("img", { src: image, alt: `${meal} image` }),
              createElement("span", {}, meal),
            ],
          ),
        ),
      ),
    );

  setTimeout(() => list.classList.remove("hide"), 100);
}

function deleteMeal() {
  closeDialog();
  const dialog = document.querySelector("#dialog");
  const mealID = dialog.dataset.mealID;
  meals.delete(mealID);

  setTimeout(() => {
    document.querySelector("#meal-list").classList.add("hide");
    setTimeout(refreshMeals, 100);
  }, 100);
}

function closeDialog() {
  document.getElementById("dialog").classList.add("hide");
  setTimeout(() => {
    document.querySelector("#dialog-content").innerHTML = "";
  }, 200);
}

async function showMealDetails(mealID) {
  const dialog = document.querySelector("#dialog");

  const container = document.querySelector("#dialog-content-container");
  container.scrollTo({ top: 0 });

  const loader = document.querySelector("#loader");
  loader.classList.remove("hidden");

  const fetchPromise = fetcher({
    url: `https://www.themealdb.com/api/json/v1/1/lookup.php?i=${mealID}`,
  });
  const sleepPromise = sleep(1000);

  const [{ status, value }] = await Promise.allSettled([
    fetchPromise,
    sleepPromise,
  ]);

  loader.classList.add("hidden");

  if (status === "rejected") return toast("Failed to fetch");

  const {
    strMeal: name,
    strMealThumb: image,
    strCategory: category,
    strInstructions: instructions,
  } = value.meals[0];

  const content = document.querySelector("#dialog-content");

  content.innerHTML = "";

  content.appendChild(
    createElement(
      "div",
      {
        id: "meal-header",
      },
      [
        createElement("img", {
          id: "meal-image",
          src: image,
          alt: `${name} image`,
          loading: "lazy",
        }),
        createElement("div", { id: "meal-info" }, [
          createElement("span", { id: "meal-name" }, name),
          createElement("span", { id: "meal-category" }, category),
        ]),
      ],
    ),
  );

  content.appendChild(
    createElement(
      "div",
      { id: "meal-instructions" },
      instructions
        .split(/\r?\n/)
        .map((line) => line.trim())
        .filter((line) => line.length > 0)
        .map((line) => createElement("p", {}, line)),
    ),
  );

  dialog.classList.remove("hide");
  dialog.dataset.mealID = mealID;
}

async function fetcher({ method, url, body }) {
  const opts = {
    method,
  };

  if (body !== undefined) {
    opts.headers = { "Content-Type": "application/json" };
    opts.body = JSON.stringify(body);
  }

  let res;

  try {
    res = await fetch(url, opts);
  } catch {
    throw new Error("Failed to fetch");
  }

  if (!res.ok) throw new Error("Failed to fetch");

  return res.headers.get("Content-Type") === "application/json"
    ? res.json()
    : res.text();
}

async function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function createElement(tag, props, children) {
  const element = document.createElement(tag);

  if (props !== undefined)
    Object.keys(props).forEach((key) => {
      element[key] = props[key];
    });

  if (children !== undefined) {
    if (Array.isArray(children))
      children.forEach((child) => element.appendChild(nodeUtil(child)));
    else element.appendChild(nodeUtil(children));
  }

  return element;
}

function nodeUtil(child) {
  return typeof child === "string" ? document.createTextNode(child) : child;
}

function toast(message, duration = 3000) {
  const toaster = document.querySelector("#toaster");

  const toast = createElement("div", { className: "toast" }, message);

  toaster.appendChild(toast);

  setTimeout(() => {
    toast.classList.add("hide");
    setTimeout(() => {
      toast.remove();
    }, 200);
  }, duration);
}
