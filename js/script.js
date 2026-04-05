import AllocatorModule from '../wasm/allocator_wasm.js';

let allocators;

async function init() {
  const Module = await AllocatorModule({
    locateFile: (path) => `../wasm/${path}`,
  });

  allocators = {
    linear: new Module.LinearAllocator(),
    'free-list': new Module.FreeListAllocator(),
    buddy: new Module.BuddyAllocator(),
  };
}

const config = {
  linear: {
    title: 'Linear Allocator',
    controls: ['allocate', 'resize_last', 'reset'],
  },
  'free-list': {
    title: 'Free List Allocator',
    controls: ['allocate', 'deallocate', 'reset'],
  },
  buddy: {
    title: 'Buddy Allocator',
    controls: ['allocate', 'deallocate', 'reset'],
  },
};

const pointers = new Map();
const SIZE = 1024;

document.addEventListener('DOMContentLoaded', async () => {
  await init();

  document.querySelectorAll('.toggle').forEach((button) => {
    button.addEventListener('click', () => {
      const type = button.dataset.type;

      const previous = document.querySelector('.selected');
      if (previous) previous.classList.remove('selected');
      button.classList.add('selected');

      renderAllocator(type);
      renderDescription(type);
      renderControls(type);
    });
  });

  function renderAllocator(type) {
    const state = JSON.parse(allocators[type].getState());

    renderBlocks(type, state);
    renderMetrics(type, state.metrics);
  }

  function renderBlocks(type, state) {
    const allocator = document.getElementById('allocator');
    const legend = document.getElementById('legend');
    const note = document.getElementById('note');

    allocator.innerHTML = '';
    allocator.className = '';

    state.blocks.forEach((block) => {
      if (block.header > 0) {
        const header = document.createElement('div');
        header.className = 'block header';
        header.style.left = `${(block.offset / state.totalBytes) * 100}%`;
        header.style.width = `${(block.header / state.totalBytes) * 100}%`;
        allocator.appendChild(header);
      }

      const element = document.createElement('div');
      element.className = `block ${block.status}`;
      element.style.left = `${((block.offset + block.header) / state.totalBytes) * 100}%`;
      element.style.width = `${(block.size / state.totalBytes) * 100}%`;

      const text = document.createElement('span');
      text.innerHTML = `${block.size} B`;
      text.style.fontSize = '0.9rem';
      element.appendChild(text);

      allocator.appendChild(element);
    });

    allocator.classList.add('view');
    legend.classList.add('view');
    note.classList.add('view');
  }

  function renderMetrics(type, metrics) {
    const items = document.querySelectorAll('.metrics-list li span');

    items.forEach((item) => {
      const key = item.id.replace('m-', '');
      item.innerHTML = `${metrics[key]}`;
    });
  }

  function renderDescription(type) {
    const info = config[type];
    document.querySelector('.title').textContent = info.title;
  }

  function renderControls(type) {
    const info = config[type];
    const controls = document.querySelector('.controls');

    const sizes = [];
    for (let i = 16; i <= SIZE / 2; i *= 2) {
      sizes.push(i);
    }

    let html = `<h4>Controls</h4>`;

    if (info.controls.includes('allocate')) {
      const options = sizes
        .map((s) => `<option value="${s}">${s} bytes</option>`)
        .join('');
      html += `
        <div class="control-row">
            <button id="allocate-button">Allocate</button>
            <select id="allocate-size">${options}</select>
        </div>
      `;
    }
    if (info.controls.includes('deallocate')) {
      html += `
        <div class="control-row">
            <button id="deallocate-button">Deallocate</button>
            <select id="deallocate-ptr">
                <option value="" disabled selected>- select block -</option>
            </select>
        </div>
      `;
    }
    if (info.controls.includes('resize_last')) {
      const options = sizes
        .map((s) => `<option value="${s}">${s} bytes</option>`)
        .join('');
      html += `
        <div class="control-row">
            <button id="resize-button">Resize Last</button>
             <select id="resize-size">${options}</select>
        </div>
      `;
    }
    if (info.controls.includes('reset')) {
      html += `
        <div class="control-row">
            <button id="reset-button">Reset</button>
        </div>
      `;
    }

    controls.innerHTML = html;
    attachControlHandlers(type);
  }

  function attachControlHandlers(type) {
    const allocateButton = document.getElementById('allocate-button');
    const deallocateButton = document.getElementById('deallocate-button');
    const resizeButton = document.getElementById('resize-button');
    const resetButton = document.getElementById('reset-button');

    if (allocateButton) {
      allocateButton.onclick = () => {
        const size = parseInt(
          document.getElementById('allocate-size').value,
          10,
        );
        const ptr = allocators[type].allocate(size);
        if (ptr === 0) {
          console.error('allocation failed');
          return;
        }

        pointers.set(ptr, size);
        syncPointerDropdown();
        renderAllocator(type);
      };
    }

    if (deallocateButton) {
      deallocateButton.onclick = () => {
        const selectedOption = document.getElementById('deallocate-ptr');
        const ptr = parseInt(selectedOption.value, 10);
        if (isNaN(ptr)) {
          console.error('could not parse pointer');
          return;
        }

        allocators[type].deallocate(ptr);
        pointers.delete(ptr);
        syncPointerDropdown();
        renderAllocator(type);
      };
    }

    if (resizeButton) {
      resizeButton.onclick = () => {
        const size = parseInt(document.getElementById('resize-size').value, 10);
        const previous_ptr = [...pointers.keys()].at(-1);
        const new_ptr = allocators[type].resizeLast(previous_ptr, size);
        if (new_ptr === 0) {
          console.error('resize allocation failed');
          return;
        }

        pointers.set(new_ptr, size);
        syncPointerDropdown();
        renderAllocator(type);
      };
    }

    if (resetButton) {
      resetButton.onclick = () => {
        allocators[type].reset();
        pointers.clear();
        syncPointerDropdown();
        renderAllocator(type);
      };
    }
  }

  function syncPointerDropdown() {
    const pointerDropdown = document.getElementById('deallocate-ptr');
    if (!pointerDropdown) {
      return;
    }

    const prev = pointerDropdown.value;
    pointerDropdown.innerHTML = `
        <option value="" disabled selected>- select block -</option>
        ${[...pointers.entries()]
          .map(
            ([ptr, size]) =>
              `<option value="${ptr}">0x${ptr.toString(16)}, (${size} bytes)</option>`,
          )
          .join('')};
    `;

    if (pointers.has(parseInt(prev))) {
      pointerDropdown.value = prev;
    }
  }
});
